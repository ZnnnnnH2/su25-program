# Performance comparison between A* and BFS
$testCases = @("283104765", "123456780", "123456708")

Write-Host "Performance Comparison: A* vs BFS"
Write-Host "====================================="

foreach ($test in $testCases) {
    Write-Host "`nTesting input: $test"
    
    # Test A* algorithm
    Write-Host "A* Algorithm:"
    $astarTime = Measure-Command { 
        $astarResult = echo $test | .\8pp-luogu-Astar.exe 
    }
    Write-Host "  Result: $astarResult"
    Write-Host "  Time: $($astarTime.TotalMilliseconds) ms"
    
    # Test BFS algorithm
    Write-Host "BFS Algorithm:"
    $bfsTime = Measure-Command { 
        $bfsResult = echo $test | .\8pp-luogu-bfs.exe 
    }
    Write-Host "  Result: $bfsResult"
    Write-Host "  Time: $($bfsTime.TotalMilliseconds) ms"
    
    # Comparison
    if ($astarTime.TotalMilliseconds -lt $bfsTime.TotalMilliseconds) {
        Write-Host "  A* is faster by $($bfsTime.TotalMilliseconds - $astarTime.TotalMilliseconds) ms"
    } else {
        Write-Host "  BFS is faster by $($astarTime.TotalMilliseconds - $bfsTime.TotalMilliseconds) ms"
    }
}
