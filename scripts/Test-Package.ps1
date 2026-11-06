param(
	[string]$BuildDirectory = 'build',
	[Parameter(Mandatory=$true)][string]$Toolset,
	[ValidateSet('x64','Win32')][string]$Architecture = 'x64',
	[switch]$MfcDemo
)
$ErrorActionPreference = 'Stop'
try {
	$build = (Resolve-Path $BuildDirectory).Path
	$arch = if ($Architecture -eq 'Win32') { 'x86' } else { 'x64' }
	# Workflow packages to repository dist. Local Build-Sdk packages to build/dist.
	$dist = Join-Path $build 'dist'
	if (!(Test-Path $dist)) { $dist = Join-Path (Split-Path $build) 'dist' }
	$zip = @(Get-ChildItem $dist -Filter "*-$arch-$Toolset.zip")
	if ($zip.Count -ne 1) { throw 'Expected one SDK ZIP.' }
	$unpacked = Join-Path $build ('package-check-' + [Guid]::NewGuid().ToString('N'))
	Expand-Archive -LiteralPath $zip[0].FullName -DestinationPath $unpacked
	$dll = @(Get-ChildItem $unpacked -Recurse -Filter GlimHalcon.dll)
	if ($dll.Count -ne 1) { throw 'Expected exactly one SDK DLL.' }
	$bin = $dll[0].DirectoryName
	foreach ($name in @('07_api_contract','GlimConsumer_Gd')) {
		Copy-Item -LiteralPath (Join-Path $build "Release/$name.exe") -Destination $bin
		& (Join-Path $bin "$name.exe")
		if ($LASTEXITCODE -ne 0) { throw "Packaged DLL consumer failed: $name" }
	}
	foreach ($name in @('API_REFERENCE.md','TOOLSET_GUIDE.md')) {
		if (!(Get-ChildItem $unpacked -Recurse -Filter $name)) { throw "Missing documentation: $name" }
	}
	if ($MfcDemo) {
		$process = Start-Process (Join-Path $bin 'GlimMfcDemo.exe') -WorkingDirectory $bin -WindowStyle Hidden -PassThru
		try {
			if (!$process.WaitForInputIdle(15000) -or $process.HasExited) { throw 'MFC startup failed.' }
			Write-Output 'Packaged MFC reached input-idle.'
		} finally {
			if (!$process.HasExited) { $null = $process.CloseMainWindow(); if (!$process.WaitForExit(5000)) { $process.Kill() } }
		}
	}
	Write-Output "Package verification passed: $arch/$Toolset"
} catch { Write-Error $_; exit 1 }
