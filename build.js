const fs = require('fs-extra');
const fsp = fs.promises;
const path = require('path');
const { exec } = require('child_process');

const execute = cmd => new Promise((resolve, reject) => {
  exec(cmd, function(err, stdout, stderr) {
    if (err) {
      reject(err);
    } else {
      resolve({stdout, stderr});
    }
  });
});

const flags = process.argv.slice(2).join(" ");

const is_spawn = flags.match(/-DSPAWN/);
const out_dir = is_spawn ? './emcc/spawn' : './emcc/retail';

let rebuild = true;
if (fs.existsSync(`${out_dir}/args.txt`)) {
  if (fs.readFileSync(`${out_dir}/args.txt`, 'utf8') === flags) {
    rebuild = false;
  }
}

const link_list = [];
let firstFile = true;
let maxTime = null;

async function handle_file(name) {
  const statSrc = await fsp.stat(name);
  if (statSrc.isDirectory()) {
    const list = await fsp.readdir(name);
    await Promise.all(list.map(fn => handle_file(`${name}/${fn}`)));
  } else if (name.match(/\.(?:c|cpp|cc)$/i)) {
    const out = `${out_dir}/${name}.bc`;
    let statDst = null;
    if (fs.existsSync(out)) {
      statDst = await fsp.stat(out);
    } else {
      fs.createFileSync(out);
    }

    if (rebuild || !statDst || statSrc.mtime > statDst.mtime) {
      if (firstFile) {
        console.log('Compiling...');
        firstFile = false;
      }
      console.log(`  ${name}`);
      const cmd = `emcc ${name} -o ${out} ${name.match(/\.(?:cpp|cc)$/i) ? "--std=c++11 " : ""}-DNO_SYSTEM -DEMSCRIPTEN -Wno-logical-op-parentheses ${flags} -I.`;
      try {
        const {stderr} = await execute(cmd);
        if (stderr) {
          console.error(stderr);
        }
      } catch (e) {
        if (fs.existsSync(out)) {
          await fsp.unlink(out);
        }
        throw e;
      }
    }

    if (!maxTime || statSrc.mtime > maxTime) {
      maxTime = statSrc.mtime;
    }

    link_list.push(out);
  }
}

async function build_all() {
  await handle_file('Source');
  fs.createFileSync(`${out_dir}/args.txt`);
  fs.writeFileSync(`${out_dir}/args.txt`, flags);

  const oname = (is_spawn ? 'DiabloSpawn' : 'Diablo');

  if (!rebuild && (!maxTime || maxTime <= fs.statSync(oname + '.wasm').mtime)) {
    console.log('Everything is up to date');
    return;    
  }

  console.log(`Linking ${is_spawn ? 'spawn' : 'retail'}`);
  const link_flags = (memSize, ex) => `${optimize} -s WASM=1 -s MODULARIZE=1 -s NO_FILESYSTEM=1 -s EXPORTED_FUNCTIONS="['_malloc', '_free']" --post-js ./module-post.js -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=${memSize}${ex ? " -s DISABLE_EXCEPTION_CATCHING=0" : ""}`;

  const {stderr} = await execute(`emcc ${link_list.join(" ")} -o ${oname}.js -s EXPORT_NAME="${oname}" ${flags} -s WASM=1 -s MODULARIZE=1 -s NO_FILESYSTEM=1 --post-js ./module-post.js -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=134217728 -s DISABLE_EXCEPTION_CATCHING=0`);
  if (stderr) {
    console.error(stderr);
  }
  fs.renameSync(oname + '.js', oname + '.jscc');
}

build_all().catch(e => console.error(e.message));
