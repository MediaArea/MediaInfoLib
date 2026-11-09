mediainfolib
==================
Native MediaInfo library wrapper for Node.js using N-API.

Install
-------
```sh
$ npm install --save mediainfolib
```

Build
-----
```sh
npm install node-addon-api
node-gyp configure
node-gyp build
```

Usage
-----
First install the MediaInfo native library on your system (see https://mediaarea.net/MediaInfo)

```js
var mod = require('mediainfolib');
var mi = mod.MediaInfo();

mi.Open('Example.ogg');
console.log(mi.Inform());
```
See example.js for more examples.

License
-------
Use of this source code is governed by a BSD-style license.
See https://mediaarea.net/en/MediaInfo/License for more information.
