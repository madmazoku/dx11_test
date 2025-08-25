@echo off
echo Compiling shaders...

fxc /T cs_5_0 /E CSMain /Fo ComputeShader.cso ComputeShader.hlsl
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile ComputeShader.hlsl
    pause
    exit /b 1
)

fxc /T vs_5_0 /E VSMain /Fo VertexShader.vso VertexShader.hlsl
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile VertexShader.hlsl
    pause
    exit /b 1
)

fxc /T gs_5_0 /E GSMain /Fo GeometryShader.gso GeometryShader.hlsl
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile GeometryShader.hlsl
    pause
    exit /b 1
)

fxc /T ps_5_0 /E PSMain /Fo PixelShader.pso PixelShader.hlsl
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile PixelShader.hlsl
    pause
    exit /b 1
)

echo All shaders compiled successfully!
pause
