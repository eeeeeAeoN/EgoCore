$wld = "D:/FableDevelopment/UnifiedFable/Data/Levels/FinalAlbion.wld"
$lines = Get-Content $wld
$inMap = $false
$mapName = ""
$mx = 0; $my = 0; $w = 0; $h = 0
foreach ($line in $lines) {
    $t = $line.Trim()
    if ($t -match '^#definition\s+\S+\s+(\S+)') {
        $mapName = $Matches[1]
        $mx = 0; $my = 0; $w = 0; $h = 0
    }
    if ($t -match '^MapPosition\s+(\d+)\s+(\d+)') {
        $mx = [int]$Matches[1]; $my = [int]$Matches[2]
    }
    if ($t -match '^MapDimensions\s+(\d+)\s+(\d+)') {
        $w = [int]$Matches[1]; $h = [int]$Matches[2]
    }
    if ($t -match '^#end_definition') {
        if ($mapName -match 'LostBay') {
            "$mapName : Pos=($mx, $my), Dim=($w, $h), Bounds=[$mx, $my, $($mx+$w), $($my+$h)]"
        }
    }
}
