WASM version is compiled with emscripten 6.x, and requires a browser
supporting WASM and JavaScript promises (Chrome 85, Firefox 79, Safari 15)
asm.js version is compiled with emscripten 1.x and has less
constraints on the required browser features but is (much) slower.

WASM related files:
- MediaInfoWasm.js
- MediaInfoWasm.wasm

asm.js related files:
- MediaInfo.js
- MediaInfo.js.mem
