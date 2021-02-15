WASM version is compiled with emscripten 2.x, and requires a browser
supporting WASM and JavaScript promises (Chrome 57, Firefox 52, Edge 16, Safari 11)
asm.js version is compiled with emscripten 1.x and has less
constraints on the required browser features but is (much) slower.

WASM related files:
- MediaInfoWasm.js
 - MediaInfoWasm.wasm

asm.js related files:
- MediaInfo.js
- MediaInfo.js.mem
