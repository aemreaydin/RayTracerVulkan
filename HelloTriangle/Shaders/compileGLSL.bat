@echo off
for /r %%i in (*.vert, *.frag) do %VULKAN_SDK%/Bin/glslc %%i -o %%~ni.spv