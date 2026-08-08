#Requires -Version 5.1
<#
.SYNOPSIS
  Machine-checked proof that A_6(5,4) = 31.

.DESCRIPTION
  .\run-all.ps1           full proof, about 4 to 5 minutes
  .\run-all.ps1 -Quick    everything except the decisive N=32 run, about 15 s
  .\run-all.ps1 -Audit    also re-run the decisive case with the hole-pattern
                          sorting break switched off (adds roughly one hour)

  Requires only a C++17 compiler (MinGW g++, clang++ or MSVC cl) and Python 3.
  No third-party libraries, no downloads, no SAT solver. Every step asserts its
  expected outcome and the script halts at the first mismatch, so a clean finish
  is itself the proof.
#>
param(
    [switch]$Quick,
    [switch]$Audit
)

$ErrorActionPreference = 'Continue'
$ProgressPreference    = 'SilentlyContinue'
try { $PSNativeCommandUseErrorActionPreference = $false } catch { }
$env:PYTHONUNBUFFERED = '1'

$Root = $PSScriptRoot
Set-Location -LiteralPath $Root
$BinDir  = Join-Path $Root 'bin'
$WorkDir = Join-Path $Root 'work'
New-Item -ItemType Directory -Force -Path $BinDir  | Out-Null
New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null

$NSteps = 8
if ($Quick) { $NSteps = 7 }
if ($Audit) { $NSteps = $NSteps + 1 }
$Script:StepNo = 0
$Stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

function Rule { Write-Host ('-' * 60) }
function Step([string]$Text) {
    $Script:StepNo++
    Write-Host ''
    Rule
    Write-Host ('[{0}/{1}] {2}' -f $Script:StepNo, $NSteps, $Text)
    Rule
}
function Note([string]$Text) { Write-Host ('  ' + $Text) }
function Fail([string]$Text) {
    Write-Host ''
    Write-Host ('#' * 60)
    Write-Host ('  FAILED at step {0}: {1}' -f $Script:StepNo, $Text)
    Write-Host '  Nothing below this point has been checked.'
    Write-Host ('#' * 60)
    exit 1
}

Write-Host ('=' * 60)
Write-Host '   A_6(5,4) = 31   --   complete machine-checked proof'
Write-Host ('=' * 60)

# ---------------------------------------------------------------- 1. toolchain
Step 'toolchain and build'

$Cxx = $null
$CxxKind = $null
foreach ($c in @('g++', 'clang++')) {
    $cmd = Get-Command $c -ErrorAction SilentlyContinue
    if ($cmd) { $Cxx = $cmd.Source; $CxxKind = 'gcc'; break }
}
if (-not $Cxx) {
    $cmd = Get-Command 'cl' -ErrorAction SilentlyContinue
    if ($cmd) { $Cxx = $cmd.Source; $CxxKind = 'msvc' }
}
if (-not $Cxx) {
    Fail 'no C++ compiler on PATH. Install MSYS2/MinGW-w64 g++, or start a Visual Studio Developer PowerShell.'
}

$PyExe = $null
$PyPre = @()
foreach ($name in @('python', 'python3', 'py')) {
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if (-not $cmd) { continue }
    $pre = @()
    if ($name -eq 'py') { $pre = @('-3') }
    & $cmd.Source @pre '-c' 'import sys; sys.exit(0 if sys.version_info[0]==3 else 1)' 2>$null
    if ($LASTEXITCODE -eq 0) { $PyExe = $cmd.Source; $PyPre = $pre + @('-u'); break }
}
if (-not $PyExe) { Fail 'no Python 3 on PATH' }

Note ('compiler : ' + $Cxx)
Note ('python   : ' + $PyExe)

$SeedsExe  = Join-Path $BinDir 'seeds.exe'
$SearchExe = Join-Path $BinDir 'searchA.exe'
$Sources   = @(
    @{ src = (Join-Path $Root 'src\seeds.cpp');   out = $SeedsExe  },
    @{ src = (Join-Path $Root 'src\searchA.cpp'); out = $SearchExe }
)
foreach ($t in $Sources) {
    if ($CxxKind -eq 'gcc') {
        & $Cxx -O2 -std=c++17 -o $($t.out) $($t.src)
    } else {
        $obj = Join-Path $BinDir ([System.IO.Path]::GetFileNameWithoutExtension($t.src) + '.obj')
        & $Cxx /nologo /O2 /EHsc /std:c++17 ('/Fe:' + $t.out) ('/Fo:' + $obj) $($t.src)
    }
    if ($LASTEXITCODE -ne 0) { Fail ('compiling ' + $t.src) }
}
Note 'built bin\seeds.exe and bin\searchA.exe'

# ------------------------------------------------------------------- 2. seeds
Step 'canonical seed lists (orbit representatives for grid row 1)'
$Seeds2 = Join-Path $WorkDir 'seeds2.txt'
$Seeds3 = Join-Path $WorkDir 'seeds3.txt'

$out2 = & $SeedsExe 2
if ($LASTEXITCODE -ne 0) { Fail 'seeds 2 failed its built-in count check' }
$out2 | Set-Content -LiteralPath $Seeds2 -Encoding ascii

$out3 = & $SeedsExe 3
if ($LASTEXITCODE -ne 0) { Fail 'seeds 3 failed its built-in count check' }
$out3 | Set-Content -LiteralPath $Seeds3 -Encoding ascii
Note 'wrote work\seeds2.txt and work\seeds3.txt'

# ------------------------------------------------------- 3. seed completeness
Step 'independent completeness check of the seed lists'
Note 'This is the one step that could silently void the upper bound,'
Note 'so it is re-derived here by a second implementation, in Python.'
Write-Host ''
& $PyExe @PyPre (Join-Path $Root 'verify\checkseeds.py') $Seeds2
if ($LASTEXITCODE -ne 0) { Fail 'S=2 seed list is not complete' }
Write-Host ''
& $PyExe @PyPre (Join-Path $Root 'verify\checkseeds.py') $Seeds3
if ($LASTEXITCODE -ne 0) { Fail 'S=3 seed list is not complete' }

# --------------------------------------------------------------- 4. certificate
Step 'lower bound: A_6(5,4) >= 31  (verify the certificate from first principles)'
& $PyExe @PyPre (Join-Path $Root 'verify\verify.py') (Join-Path $Root 'certs\w31.txt') 6 4
if ($LASTEXITCODE -ne 0) { Fail 'certs\w31.txt is not a valid (5,31,4)_6 code' }

# ---------------------------------------------------------------- 5,6. controls
Step 'control against the literature: a 34-word code at K=4 exists'
Note 'A_6(4,3) = 34 is known independently; the search must find one.'
& $SearchExe 2 34 $Seeds2 -expect F -every 5
if ($LASTEXITCODE -ne 0) { Fail 'K=4 N=34 did not come out FEASIBLE' }

Step 'control against the literature: no 35-word code at K=4 exists'
Note 'the same known value from the other side; the search must refute it.'
& $SearchExe 2 35 $Seeds2 -expect I -every 5
if ($LASTEXITCODE -ne 0) { Fail 'K=4 N=35 did not come out INFEASIBLE' }

# ----------------------------------------------------------- 7. positive control
Step 'positive control at K=5: a 30-word code exists'
Note 'guards against a search that refutes everything.'
& $SearchExe 3 30 $Seeds3 -expect F -every 5
if ($LASTEXITCODE -ne 0) { Fail 'K=5 N=30 did not come out FEASIBLE' }

# ------------------------------------------------------------------ 8. decisive
$SumPath = Join-Path $WorkDir 'n32.sum'
if (-not $Quick) {
    Step 'DECISIVE: no 32-word code at K=5  (this is the upper bound)'
    Note '103 seeds x 696 hole patterns, exhaustive. Expect about 4 minutes.'
    Write-Host ''
    & $SearchExe 3 32 $Seeds3 -expect I -sum $SumPath -every 2
    if ($LASTEXITCODE -ne 0) { Fail 'K=5 N=32 did not come out INFEASIBLE' }
}

# --------------------------------------------------------------------- 9. audit
if ($Audit) {
    Step 'audit: same case with the hole-pattern sorting break disabled'
    Note 'enumerates all C(24,4) = 10626 patterns instead of the 696 sorted ones.'
    Note 'This takes roughly an hour and is not needed for the proof; the break'
    Note 'is justified by a counting argument in PROOF.md section 4.'
    Write-Host ''
    & $SearchExe 3 32 $Seeds3 -allpat -expect I -sum (Join-Path $WorkDir 'n32_allpat.sum') -every 10
    if ($LASTEXITCODE -ne 0) { Fail 'the -allpat audit did not come out INFEASIBLE' }
}

# -------------------------------------------------------------------- summary
$Stopwatch.Stop()
$Elapsed = [int]$Stopwatch.Elapsed.TotalSeconds

$sum = @{}
if (Test-Path -LiteralPath $SumPath) {
    foreach ($line in Get-Content -LiteralPath $SumPath) {
        if ($line -match '^([A-Za-z_]+)=(.*)$') { $sum[$Matches[1]] = $Matches[2] }
    }
}

Write-Host ''
Write-Host ''
Write-Host ('=' * 60)
if ($Quick) {
    Write-Host '   QUICK MODE -- the decisive run was skipped'
    Write-Host ('=' * 60)
    Write-Host '   Everything except the N=32 refutation passed.'
    Write-Host '   Run .\run-all.ps1 with no flags for the actual proof.'
    Write-Host ('   elapsed: {0}s' -f $Elapsed)
    Write-Host ('=' * 60)
    exit 0
}
Write-Host '   PROOF COMPLETE        A_6(5,4) = pa(5;6) = 31'
Write-Host ('=' * 60)
Write-Host '   lower bound   >= 31   certs\w31.txt verified from scratch:'
Write-Host '                         465 pairs, minimum Hamming distance 4'
Write-Host '   upper bound   <= 31   no 32-word code exists:'
Write-Host ('                         {0} seeds x {1} hole patterns = {2} subproblems' -f $sum['sum_seeds'], $sum['sum_patterns'], $sum['sum_subproblems'])
Write-Host ('                         {0} nodes, {1}s, {2}' -f $sum['sum_nodes'], $sum['sum_time'], $sum['sum_status'])
Write-Host '   reduction             103 orbit representatives cover all 393120'
Write-Host '                         4x6 Latin rectangles: 0 missing, 0 spurious'
Write-Host '   controls              A_6(4,3) = 34 reproduced from both sides;'
Write-Host '                         30-word code at K=5 found (positive control)'
Write-Host ('   total wall clock      {0}s' -f $Elapsed)
Write-Host ('=' * 60)
Write-Host '   Reference run: 1427441869 nodes. The search is deterministic,'
Write-Host '   so the node count above should match exactly on any machine.'
Write-Host ('=' * 60)
exit 0
