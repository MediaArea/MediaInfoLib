@rem echo off

@rem MS Visual Studio specific ---
rmdir Win32 /Q /S
rmdir x64/Q /S
del *.ncb *.user
del *.suo /AH