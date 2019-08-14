const WebSocket = require('ws');
const {read_packet, write_packet, server_packet, client_packet, buffer_reader, buffer_writer, RejectionReason} = require('./packet');

const wss = new WebSocket.Server({ port: 3001 });

const MAX_PLRS = 4;
const SERVER_VERSION = 1;

const games = {};

function parse_packet(buffer, types) {
  const reader = new buffer_reader(buffer);
  const result = read_packet(reader, types);
  if (!reader.done()) {
    throw Error('packet too large');
  }
  return result;
}

class game_instance {
  constructor(version, name, password, difficulty) {
    this.players = [];
    this.version = version;
    this.name = name;
    this.password = password;
    this.difficulty = difficulty;
    this.seed = Math.floor(Math.random() * (2 ** 32));
  }

  num_players() {
    let count = 0;
    for (let i = 0; i < MAX_PLRS; ++i) {
      if (this.players[i]) {
        ++count;
      }
    }
    return count;
  }
  free_index() {
    for (let i = 0; i < MAX_PLRS; ++i) {
      if (!this.players[i]) {
        return i;
      }
    }
    return null;
  }

  send(mask, packet) {
    const players = this.players;
    for (let i = 0; i < MAX_PLRS; ++i) {
      if (players[i] && (mask & (1 << i))) {
        players[i].send(packet);
      }
    }
  }

  drop_player(id, reason) {
    this.send(0xFF, write_packet(server_packet.disconnect, {id, reason}));
    if (this.players[id]) {
      this.players[id].on_leave();
    }
    this.players[id] = null;
    if (!this.num_players()) {
      delete games[this.name.toLowerCase()];
    }
  }

  join(player, cookie) {
    let id = this.free_index();
    if (id == null) {
      player.send(write_packet(server_packet.join_reject, {cookie, reason: RejectionReason.JOIN_GAME_FULL}));
    } else {
      this.players[id] = player;
      player.send(write_packet(server_packet.join_accept, {cookie, index: id, seed: this.seed, difficulty: this.difficulty}));
      this.send(0xFF, write_packet(server_packet.connect, {id}));
    }
    return id;
  }
}

function create_game(version, name, password, difficulty) {
  const lwr = name.toLowerCase();
  if (games[lwr]) {
    return RejectionReason.CREATE_GAME_EXISTS;
  }
  games[lwr] = new game_instance(version, name, password, difficulty);
  return 0;
}

class player_client {
  constructor(ws) {
    this.batch = [];
    this.ws = ws;
    this.ws.send(write_packet(server_packet.info, {version: SERVER_VERSION}));
    ws.on('message', this.on_message.bind(this));
    ws.on('close', this.on_close.bind(this));
    ws.on('pong', () => this.alive = true);
    this.alive = true;
    this.version = 0;

    this.interval = setInterval(() => {
      if (!this.alive) {
        this.ws.terminate();
        this.on_close();
      }
      this.alive = false;
      this.ws.ping(() => {});
    }, 15000);

    this.sendInterval = setInterval(() => {
      if (this.batch.length) {
        const writer = new buffer_writer(this.batch.reduce((sum, buffer) => sum + buffer.byteLength, 3));
        writer.write8(0);
        writer.write16(this.batch.length);
        for (let buffer of this.batch) {
          writer.rest(new Uint8Array(buffer));
        }
        this.ws.send(writer.result);
        this.batch.length = 0;
      }
    }, 200);
  }

  send(msg) {
    this.batch.push(msg);
    //this.ws.send(msg);
  }

  join(cookie, name, password) {
    const game = games[name.toLowerCase()];
    if (!game) {
      this.send(write_packet(server_packet.join_reject, {cookie, reason: RejectionReason.JOIN_GAME_NOT_FOUND}));
    } else if (game.version !== this.version) {
      this.send(write_packet(server_packet.join_reject, {cookie, reason: RejectionReason.JOIN_VERSION_MISMATCH}));
    } else if (game.password !== password) {
      this.send(write_packet(server_packet.join_reject, {cookie, reason: RejectionReason.JOIN_INCORRECT_PASSWORD}));
    } else {
      let id = game.join(this, cookie);
      if (id != null) {
        this.game = game;
        this.id = id;
      }
    }
  }

  handle(code, pkt) {
    switch (code) {
    case client_packet.batch.code:
      for (let {type, packet} of pkt) {
        this.handle(type.code, packet);
      }
      break;
    case client_packet.info.code:
      this.version = pkt.version;
      break;
    case client_packet.game_list.code:
      this.send(write_packet(server_packet.game_list, {games: Object.values(games).map(game => ({name: game.name, type: game.difficulty | (game.password ? 0x80000000 : 0)}))}));
      break;
    case client_packet.create_game.code:
      if (this.game) {
        this.send(write_packet(server_packet.join_reject, {cookie: pkt.cookie, reason: RejectionReason.JOIN_ALREADY_IN_GAME}));
      } else {
        const reason = create_game(this.version, pkt.name, pkt.password, pkt.difficulty);
        if (reason) {
          this.send(write_packet(server_packet.join_reject, {cookie: pkt.cookie, reason}));
        } else {
          this.join(pkt.cookie, pkt.name, pkt.password);
        }
      }
      break;
    case client_packet.join_game.code:
      if (this.game) {
        this.send(write_packet(server_packet.join_reject, {cookie: pkt.cookie, reason: RejectionReason.JOIN_ALREADY_IN_GAME}));
      } else {
        this.join(pkt.cookie, pkt.name, pkt.password);
      }
      break;
    case client_packet.leave_game.code:
      if (this.game) {
        this.game.drop_player(this.id, 3);
      }
      break;
    case client_packet.drop_player.code:
      if (this.game) {
        this.game.drop_player(pkt.id, pkt.reason);
      }
      break;
    case client_packet.message.code:
      if (this.game) {
        this.game.send(pkt.id === 0xFF ? ~(1 << this.id) : (1 << pkt.id), write_packet(server_packet.message, {id: this.id, payload: pkt.payload}));
      }
      break;
    case client_packet.turn.code:
      if (this.game) {
        this.game.send(~(1 << this.id), write_packet(server_packet.turn, {id: this.id, turn: pkt.turn}));
      }
      break;
    }
  }

  on_message(buffer) {
    try {
      const {type, packet} = parse_packet(buffer, client_packet);
      this.handle(type.code, packet);
    } catch (e) {
      console.error(e);
      this.ws.terminate();
      this.on_close();
    }
  }

  on_close() {
    if (this.game) {
      this.game.drop_player(this.id, 0x40000006);
    }
    if (this.interval) {
      clearInterval(this.interval);
      delete this.interval;
    }
    if (this.sendInterval) {
      clearInterval(this.sendInterval);
      delete this.sendInterval;
    }
  }

  on_leave() {
    this.game = null;
    this.id = null;
  }
}

wss.on('connection', ws => {
  new player_client(ws);
});
