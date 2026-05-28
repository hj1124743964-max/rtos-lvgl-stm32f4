param(
    [Parameter(Mandatory = $true)]
    [string]$Source
)

$tmp = Join-Path $env:TEMP ('xlsx_extract_' + [guid]::NewGuid().ToString())
$zip = Join-Path $env:TEMP ('xlsx_extract_' + [guid]::NewGuid().ToString() + '.zip')

New-Item -ItemType Directory -Path $tmp | Out-Null
Copy-Item -LiteralPath $Source -Destination $zip
Expand-Archive -LiteralPath $zip -DestinationPath $tmp

Write-Output '--- workbook ---'
Get-Content -LiteralPath (Join-Path $tmp 'xl\workbook.xml') -Encoding UTF8

Write-Output '--- shared strings ---'
$sstPath = Join-Path $tmp 'xl\sharedStrings.xml'
if (Test-Path $sstPath) {
    [xml]$sst = Get-Content -LiteralPath $sstPath -Encoding UTF8
    $ns = New-Object System.Xml.XmlNamespaceManager($sst.NameTable)
    $ns.AddNamespace('m', 'http://schemas.openxmlformats.org/spreadsheetml/2006/main')
    $nodes = $sst.SelectNodes('//m:si', $ns)
    Write-Output ("count={0}" -f $nodes.Count)
    for ($i = 0; $i -lt $nodes.Count; $i++) {
        $text = ($nodes[$i].SelectNodes('.//m:t', $ns) | ForEach-Object { $_.InnerText }) -join ''
        Write-Output ("[{0}] {1}" -f $i, $text)
    }
}

Remove-Item -LiteralPath $zip -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $tmp -Recurse -Force -ErrorAction SilentlyContinue
