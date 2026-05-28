$src = 'g:\v2.0\Hardware\OpenArm 2.0\OpenArm 2.0 BOM.xlsx'
$outDir = 'E:\rtos_lvgl_machine\translated'
$out = Join-Path $outDir 'OpenArm_2.0_BOM_CN.xlsx'
$translationFile = 'E:\rtos_lvgl_machine\tools\openarm_translations.tsv'
$tmp = Join-Path $env:TEMP ('xlsx_translate_' + [guid]::NewGuid().ToString())
$zipIn = Join-Path $env:TEMP ('xlsx_translate_' + [guid]::NewGuid().ToString() + '.zip')
$zipOut = Join-Path $env:TEMP ('xlsx_translate_out_' + [guid]::NewGuid().ToString() + '.zip')

New-Item -ItemType Directory -Force -Path $outDir | Out-Null
New-Item -ItemType Directory -Path $tmp | Out-Null
Copy-Item -LiteralPath $src -Destination $zipIn
Expand-Archive -LiteralPath $zipIn -DestinationPath $tmp

$translations = @{}
foreach ($line in [System.IO.File]::ReadAllLines($translationFile, [System.Text.Encoding]::UTF8)) {
    if ([string]::IsNullOrWhiteSpace($line)) { continue }
    $parts = $line -split "`t", 2
    if ($parts.Count -eq 2) {
        $translations[[int]$parts[0]] = $parts[1].Replace('\n', "`n").TrimEnd("`r")
    }
}

$sstPath = Join-Path $tmp 'xl\sharedStrings.xml'
[xml]$sst = Get-Content -LiteralPath $sstPath -Encoding UTF8
$ns = New-Object System.Xml.XmlNamespaceManager($sst.NameTable)
$ns.AddNamespace('m', 'http://schemas.openxmlformats.org/spreadsheetml/2006/main')
$nodes = $sst.SelectNodes('//m:si', $ns)

for ($i = 0; $i -lt $nodes.Count; $i++) {
    if (-not $translations.ContainsKey($i)) { continue }
    $si = $nodes[$i]
    while ($si.HasChildNodes) { [void]$si.RemoveChild($si.FirstChild) }
    $t = $sst.CreateElement('t', 'http://schemas.openxmlformats.org/spreadsheetml/2006/main')
    $t.SetAttribute('xml:space', 'preserve')
    $t.InnerText = $translations[$i]
    [void]$si.AppendChild($t)
}

$settings = New-Object System.Xml.XmlWriterSettings
$settings.Encoding = New-Object System.Text.UTF8Encoding($false)
$settings.Indent = $false
$writer = [System.Xml.XmlWriter]::Create($sstPath, $settings)
$sst.Save($writer)
$writer.Close()

if (Test-Path $zipOut) { Remove-Item -LiteralPath $zipOut -Force }
Compress-Archive -Path (Join-Path $tmp '*') -DestinationPath $zipOut
Copy-Item -LiteralPath $zipOut -Destination $out -Force

Remove-Item -LiteralPath $zipIn -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $zipOut -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $tmp -Recurse -Force -ErrorAction SilentlyContinue

Get-Item $out | Select-Object FullName, Length, LastWriteTime
