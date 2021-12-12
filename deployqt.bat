windeployqt.exe .\install\vls.exe
move %cd%\install\bin\*.* %cd%\install
rmdir %cd%\install\bin
pause