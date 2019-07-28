const fs = require('fs');
const path = require('path');

const optimize = '-O1';
const flags = '-O1';

function BUILD(spawn) {

  const exclude = [
    /_asm\.cpp/,
    /_render\.cpp/,
    /appfat\.cpp/,
    /dthread\.cpp/,
    /fault\.cpp/,
    /init\.cpp/,
    /logging\.cpp/,
    /nthread\.cpp/,
    /restrict\.cpp/,
    /sound\.cpp/,
  ];

  const out = [];
  const names = {};

  const compile_flags = isCpp => `${isCpp ? "--std=c++11 " : ""}${flags} -DNO_SYSTEM -DEMSCRIPTEN -DZ_SOLO${spawn ? ' -DSPAWN' : ''} -Wno-logical-op-parentheses -I.`;
  const link_flags = (memSize, ex) => `${optimize} -s WASM=1 -s MODULARIZE=1 -s NO_FILESYSTEM=1 -s EXPORTED_FUNCTIONS="['_malloc', '_free']" --post-js ./module-post.js -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=${memSize}${ex ? " -s DISABLE_EXCEPTION_CATCHING=0" : ""}`;

  function mkfile(file) {
    if (exclude.some(name => file.match(name))) {
      return [];
    }
    const p = path.parse(file);
    const name = `${spawn ? 'emcc_spawn' : 'emcc'}/${p.name}${names[p.name] || ""}.bc`;
    out.push(`call emcc ${file} -o ${name} ${compile_flags(p.ext.toLowerCase() === ".cpp")}`);
    names[p.name] = (names[p.name] || 0) + 1;
    return [name];
  }

  function mkdir(dir, rec) {
    const res = [];
    fs.readdirSync(dir).forEach(name => {
      const fn = path.join(dir, name), ext = path.extname(name);
      if (fs.lstatSync(fn).isDirectory()) {
        if (rec) res.push(...mkdir(fn, rec));
      } else if ([".c", ".cpp"].indexOf(ext.toLowerCase()) >= 0) {
        res.push(...mkfile(fn));
      }
    });
    return res;
  }

  const common = [];
  const parser = [];
  const image = [];

  parser.push(...mkdir('Source', true));

  const oname = (spawn ? 'DiabloSpawn' : 'Diablo');

  const res = `${out.join("\n")}

call emcc ${parser.join(" ")} -o ${oname}.js -s EXPORT_NAME="${oname}" ${link_flags(spawn ? 268435456 : 1073741824, true)}
move ${oname}.js ${oname}.jscc
`;

  fs.writeFileSync(spawn ? 'build_spawn.bat' : 'build.bat', res);
}
BUILD(true);
BUILD(false);
