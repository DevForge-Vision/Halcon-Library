param(
	[Parameter(Mandatory=$true)][string]$Installer,
	[Parameter(Mandatory=$true)][string]$Zip,
	[Parameter(Mandatory=$true)][string]$Checksums,
	[switch]$MfcDemo
)
$ErrorActionPreference = 'Stop'

# Never elevate or install on a developer workstation. Use an expendable CI runner.
if ($env:GITHUB_ACTIONS -ne 'true' -or !$env:RUNNER_TEMP) {
	throw 'Installer verification requires an expendable GitHub Actions Windows runner.'
}
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
	throw 'Runner must already be elevated; this script never requests elevation.'
}

function Invoke-CheckedProcess([string]$File, [string]$Arguments) {
	$process = Start-Process -FilePath $File -ArgumentList $Arguments -WindowStyle Hidden -PassThru
	if (!$process.WaitForExit(120000)) {
		$process.Kill()
		throw "Process timed out: $File"
	}
	if ($process.ExitCode -ne 0) { throw "Process failed ($($process.ExitCode)): $File" }
}

$root = Join-Path $env:RUNNER_TEMP ('glim-installer-' + [Guid]::NewGuid().ToString('N'))
$destination = Join-Path $root 'SDK With Spaces'
$expected = Join-Path $root 'expected'
$report = [ordered]@{ installer = ''; checksum = $false; installedFiles = 0; registryVerified = $false; mfcStarted = $false; uninstalled = $false; environment = 'GitHub hosted Windows runner (not a clean PC)' }
try {
	$installerPath = (Resolve-Path -LiteralPath $Installer).Path
	$zipPath = (Resolve-Path -LiteralPath $Zip).Path
	$report.installer = [IO.Path]::GetFileName($installerPath)
	if ($report.installer -notmatch '^GlimHalconSDK-([0-9]+\.[0-9]+\.[0-9]+)-(x64|x86)-(v14[0-3])\.exe$') { throw 'Unexpected installer name.' }
	$registryName = "GlimHalconSDK-$($Matches[1])-$($Matches[2])-$($Matches[3])"
	$registryPaths = @(
		"HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$registryName",
		"HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\$registryName",
		"HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$registryName",
		"HKCU:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\$registryName"
	)
	foreach ($path in $registryPaths) {
		if (Test-Path -LiteralPath $path) { throw 'Existing installation detected; refusing to replace it.' }
	}
	$manifest = Get-Content -LiteralPath $Checksums
	foreach ($file in @($installerPath, $zipPath)) {
		$name = [IO.Path]::GetFileName($file)
		$entries = @($manifest | Where-Object { $_ -match ('^[0-9a-fA-F]{64}  ' + [regex]::Escape($name) + '$') })
		if ($entries.Count -ne 1) { throw "Missing/duplicate checksum: $name" }
		$actual = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash
		if ($actual -ne $entries[0].Substring(0,64)) { throw "Checksum mismatch: $name" }
	}
	$report.checksum = $true
	$null = New-Item -ItemType Directory -Path $root
	Expand-Archive -LiteralPath $zipPath -DestinationPath $expected
	$dlls = @(Get-ChildItem -LiteralPath $expected -Recurse -Filter GlimHalcon.dll)
	if ($dlls.Count -ne 1) { throw 'Expected one DLL in ZIP.' }
	# CPack ZIP has a single top-level SDK directory.
	$packageRoots = @(Get-ChildItem -LiteralPath $expected -Directory)
	if ($packageRoots.Count -ne 1 -or !(Test-Path (Join-Path $packageRoots[0].FullName 'include/GlimImage.h'))) {
		throw 'Unexpected ZIP layout.'
	}
	# NSIS requires /D to be last and unquoted, even for paths containing spaces.
	Invoke-CheckedProcess $installerPath "/S /D=$destination"
	$registered = @($registryPaths | Where-Object { Test-Path -LiteralPath $_ })
	if ($registered.Count -ne 1) { throw 'Expected one installed Programs and Features entry.' }
	$uninstallCommand = (Get-ItemProperty -LiteralPath $registered[0]).UninstallString
	if ($uninstallCommand -ne ('"' + (Join-Path $destination 'Uninstall.exe') + '"')) { throw 'Registered uninstaller path differs.' }
	$report.registryVerified = $true
	$packageRoot = $packageRoots[0].FullName
	foreach ($file in Get-ChildItem -LiteralPath $packageRoot -Recurse -File) {
		$relative = $file.FullName.Substring($packageRoot.Length + 1)
		$installed = Join-Path $destination $relative
		if (!(Test-Path -LiteralPath $installed -PathType Leaf)) { throw "Installed file missing: $relative" }
		if ((Get-FileHash -LiteralPath $installed).Hash -ne (Get-FileHash -LiteralPath $file.FullName).Hash) {
			throw "Installed file differs from ZIP: $relative"
		}
		$report.installedFiles++
	}
	$demo = @(Get-ChildItem -LiteralPath $destination -Recurse -Filter GlimMfcDemo.exe)
	if ($MfcDemo) {
		if ($demo.Count -ne 1) { throw 'Expected installed MFC demo.' }
		$process = Start-Process $demo[0].FullName -WorkingDirectory $demo[0].DirectoryName -WindowStyle Hidden -PassThru
		try {
			if (!$process.WaitForInputIdle(15000) -or $process.HasExited) { throw 'Installed MFC startup failed.' }
			$report.mfcStarted = $true
		} finally {
			if (!$process.HasExited) {
				$null = $process.CloseMainWindow()
				if (!$process.WaitForExit(5000)) { $process.Kill(); $process.WaitForExit() }
			}
		}
	} elseif ($demo.Count -ne 0) { throw 'Engine-only installer unexpectedly contains MFC demo.' }
	$uninstaller = Join-Path $destination 'Uninstall.exe'
	if (!(Test-Path -LiteralPath $uninstaller)) { throw 'Missing uninstaller.' }
	# Copy outside the install directory and use NSIS _?= to wait for the real process.
	$uninstallCopy = Join-Path $root 'Uninstall.exe'
	Copy-Item -LiteralPath $uninstaller -Destination $uninstallCopy
	Invoke-CheckedProcess $uninstallCopy "/S _?=$destination"
	if (Test-Path -LiteralPath $destination) { throw 'Uninstall left the installation directory.' }
	foreach ($path in $registryPaths) {
		if (Test-Path -LiteralPath $path) { throw "Uninstall left registry entry: $path" }
	}
	$report.uninstalled = $true
} finally {
	$report | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $env:RUNNER_TEMP 'installer-result.json') -Encoding utf8
	$report | ConvertTo-Json | Write-Output
}
