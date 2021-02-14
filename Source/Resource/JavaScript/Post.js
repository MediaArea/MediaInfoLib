helpers = function() {
  MediaInfoLib.MediaInfo.prototype.Open = function(file, callback) {
    var _this = this, offset = 0, CHUNK_SIZE = 1024 * 1024;

    if (file.constructor !== File)
      return; //wrong argument

    _this.Option('File_FileName', file.name);
    _this.Open_Buffer_Init(file.size, 0);

    var loop = function(length) {
      var r = new FileReader();
      var blob = file.slice(offset, offset + length);
      r.onload = processChunk;
      r.readAsArrayBuffer(blob);
    };

    var processChunk = function(e) {
      if (e.target.error === null) {
        var state = _this.Open_Buffer_Continue(e.target.result);

        //Test if there is a MediaInfo request to go elsewhere
        var seekTo = _this.Open_Buffer_Continue_Goto_Get();
        if(seekTo === -1) {
          offset += e.target.result.byteLength;
        } else {
          offset = seekTo;
          _this.Open_Buffer_Init(file.size, seekTo); // Inform MediaInfo we have seek
        }
      } else {
        typeof callback==="function"&&callback(); // Error
        return;
      }

      // Bit 3 set means finalized
      if (state&0x08 || e.target.result.byteLength < 1) {
        _this.Open_Buffer_Finalize();
        typeof callback==="function"&&callback();
        return;
      }
      loop(CHUNK_SIZE);
    };

    // Start
    loop(CHUNK_SIZE);
  };
};

if (typeof MediaInfoLib.ready !== 'undefined') {
  MediaInfoLib.ready.then(helpers);
}
else {
  addOnPostRun(helpers);
}
