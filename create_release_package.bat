cd ..

rmdir PoliceLightDetectSource /Q /S
mkdir PoliceLightDetectSource

rmdir PoliceLightDetect\PoliceLightDetect\obj /Q /S
rmdir PoliceLightDetect\lib\opencv320\3rdparty\compiled_lib\Debug-X64 /Q /S
rmdir PoliceLightDetect\lib\opencv320\3rdparty\compiled_lib\Release-X64 /Q /S
del PoliceLightDetect\bin\*.pdb /F /Q
del PoliceLightDetect\bin\*.ilk /F /Q
del PoliceLightDetect\bin\*.exe /F /Q
del PoliceLightDetect\bin\*.iobj /F /Q
del PoliceLightDetect\bin\*.ipdb /F /Q
del PoliceLightDetect\bin\*.lib /F /Q
del PoliceLightDetect\bin\*.exp /F /Q
del PoliceLightDetect\*.db /F /Q
del PoliceLightDetectSource.zip /F /Q
del PoliceLightDetect\lib\opencv320\3rdparty\ippicv\ippicv_win\lib\intel64\ippicvmt.lib /F /Q

xcopy PoliceLightDetect\* PoliceLightDetectSource /E

rmdir PoliceLightDetectSource\.svn /Q /S
rmdir PoliceLightDetectSource\lib\opencv320\.svn /Q /S
rmdir PoliceLightDetectsource\lib\opencv320\apps /q /s
rmdir PoliceLightDetectsource\lib\opencv320\bin /q /s
rmdir PoliceLightDetectsource\lib\opencv320\doc /q /s
rmdir PoliceLightDetectsource\lib\opencv320\platforms /q /s
rmdir PoliceLightDetectsource\lib\opencv320\samples /q /s
rmdir PoliceLightDetectsource\lib\opencv320\modules\calib3d /q /s
rmdir PoliceLightDetectsource\lib\opencv320\modules\java /q /s
rmdir PoliceLightDetectsource\lib\opencv320\modules\ml /q /s
rmdir PoliceLightDetectsource\lib\opencv320\modules\objdetect /q /s
rmdir PoliceLightDetectsource\lib\opencv320\modules\photo /q /s
rmdir PoliceLightDetectsource\lib\opencv320\modules\python /q /s
rmdir PoliceLightDetectsource\lib\opencv320\modules\python2 /q /s
rmdir PoliceLightDetectsource\lib\opencv320\modules\shape /q /s
rmdir PoliceLightDetectsource\lib\opencv320\modules\stitching /q /s
rmdir PoliceLightDetectsource\lib\opencv320\modules\superres /q /s
rmdir PoliceLightDetectsource\lib\opencv320\modules\ts /q /s
rmdir PoliceLightDetectsource\lib\opencv320\modules\video /q /s
rmdir PoliceLightDetectsource\lib\opencv320\modules\videostab /q /s
rmdir PoliceLightDetectsource\lib\opencv320\modules\viz /q /s
rmdir PoliceLightDetectsource\lib\opencv320\modules\world /q /s
rmdir PoliceLightDetectsource\lib\opencv320\contrib /q /s
rmdir PoliceLightDetectsource\lib\opencv320\3rdparty\ffmpeg /q /s
rmdir PoliceLightDetectsource\lib\opencv320\3rdparty\lib\debug-x64 /q /s
rmdir PoliceLightDetectsource\lib\opencv320\3rdparty\lib\release-x64 /q /s

rmdir PoliceLightDetectSource\lib\Debug /Q /S
rmdir PoliceLightDetectSource\lib\Release /Q /S

del PoliceLightDetectSource\create_release_package.bat

"C:\Program Files\7-Zip\7z.exe" a PoliceLightDetectSource.zip PoliceLightDetectSource\

pause