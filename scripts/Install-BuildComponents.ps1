param([ValidateSet('v140','v141','v142','v143')][string]$Toolset = 'v143')
$ErrorActionPreference = 'Stop'
try {
	$installerRoot = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer'
	$vswhere = Join-Path $installerRoot 'vswhere.exe'
	$installPath = & $vswhere -latest -products '*' -version '[17.0,18.0)' -property installationPath
	if (!$installPath) { throw 'Visual Studio 2022 not found.' }
	$components = @('Microsoft.VisualStudio.Component.Windows10SDK.19041')
	switch ($Toolset) {
		'v140' { $components += 'Microsoft.VisualStudio.Component.VC.140' }
		'v141' { $components += 'Microsoft.VisualStudio.Component.VC.v141.x86.x64', 'Microsoft.VisualStudio.Component.VC.v141.MFC' }
		'v142' { $components += 'Microsoft.VisualStudio.ComponentGroup.VC.Tools.142.x86.x64', 'Microsoft.VisualStudio.Component.VC.14.29.16.11.MFC' }
		'v143' { $components += 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', 'Microsoft.VisualStudio.Component.VC.ATLMFC' }
	}
	$arguments = @('modify', '--installPath', ('"' + $installPath + '"'), '--quiet', '--norestart')
	foreach ($component in $components) { $arguments += @('--add', $component) }
	$process = Start-Process (Join-Path $installerRoot 'setup.exe') -ArgumentList $arguments -WindowStyle Hidden -PassThru -Wait
	if ($process.ExitCode -notin @(0,3010)) { throw "Component installation failed: $($process.ExitCode)" }
	Write-Output "Installed $Toolset components; exit=$($process.ExitCode)"
} catch { Write-Error $_; exit 1 }
