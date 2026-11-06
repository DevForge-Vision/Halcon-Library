param(
	[string]$Generator = 'Visual Studio 17 2022',
	[ValidateSet('default', 'v140', 'v141', 'v142', 'v143', 'v145')]
	[string]$Toolset = 'default',
	[ValidateSet('x64', 'Win32')][string]$Architecture = 'x64',
	[switch]$MfcDemo,
	[switch]$Package,
	[switch]$ConfigureOnly,
	[string]$CMake = 'cmake'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
try {
	$source = Split-Path $PSScriptRoot -Parent
	$generatorKey = $Generator -replace '[^a-zA-Z0-9]', '-'
	$build = Join-Path $source "build-sdk-$generatorKey-$Toolset-$Architecture"
	$cmakePath = (Get-Command $CMake -ErrorAction Stop).Source
	$ctest = Join-Path (Split-Path $cmakePath) 'ctest.exe'
	$cpack = Join-Path (Split-Path $cmakePath) 'cpack.exe'
	$configureArgs = @('-S', $source, '-B', $build, '-G', $Generator, '-A', $Architecture,
		'-DGLIM_HALCON_BUILD_SHARED=ON',
		"-DGLIM_HALCON_BUILD_MFC_DEMO=$($MfcDemo.IsPresent)",
		"-DGLIM_HALCON_ENABLE_PACKAGING=$($Package.IsPresent)")
	if ($Toolset -ne 'default') { $configureArgs += @('-T', $Toolset) }
	& $cmakePath @configureArgs
	if ($LASTEXITCODE -ne 0) { throw 'Configure failed. Install the selected toolset/SDK (and MFC if requested).' }
	if ($ConfigureOnly) { return }
	& $cmakePath --build $build --config Release --parallel
	if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
	& $ctest --test-dir $build -C Release --output-on-failure --no-tests=error
	if ($LASTEXITCODE -ne 0) { throw 'Tests failed. Packaging is blocked.' }
	if ($Package) {
		& $cpack --config (Join-Path $build 'CPackConfig.cmake') -C Release -G 'NSIS;ZIP' -B (Join-Path $build 'dist')
		if ($LASTEXITCODE -ne 0) { throw 'Packaging failed. Check NSIS and redistributables.' }
	}
} catch {
	Write-Error $_
	exit 1
}
