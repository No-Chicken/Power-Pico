@echo off
echo Cleaning Keil MDK build files...

:: 删除可能存在的标准输出文件夹 (2>nul 用于屏蔽找不到文件时的报错提示)
rd /s /q Objects 2>nul
rd /s /q Listings 2>nul
rd /s /q DebugConfig 2>nul
:: CubeMX 默认经常会生成一个与工程同名的文件夹存放编译垃圾
rd /s /q %~n0 2>nul

:: 删除散落的中间文件。保留了长名单，但剔除了致命的 .sct 和 .scvd文件，以免误删重要的链接脚本和调试配置文件。
del *.o /s /q 2>nul
del *.d /s /q 2>nul
del *.crf /s /q 2>nul
del *.axf /s /q 2>nul
del *.map /s /q 2>nul
del *.htm /s /q 2>nul
del *.lnp /s /q 2>nul
del *.lst /s /q 2>nul
del *.dep /s /q 2>nul
del *.__i /s /q 2>nul
del *.iex /s /q 2>nul
del *.tra /s /q 2>nul
del *.bak /s /q 2>nul
del *.tmp /s /q 2>nul
del *.hex /s /q 2>nul
del *.bin /s /q 2>nul

:: 清理用户界面的历史缓存和调试日志
del *.uvguix.* /s /q 2>nul
del JLinkLog.txt /s /q 2>nul

echo.
echo Clean done! Your MDK-ARM folder is now clean.
pause
