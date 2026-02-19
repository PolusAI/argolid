@echo off
REM before_all_win.bat
REM Replaces the inline CIBW_BEFORE_ALL_WINDOWS command for wheel-build jobs.
REM Expects:
REM   prereq_cache\local_install — populated by prebuild job (may be absent on cache miss)

if exist prereq_cache\local_install (
    echo =^> Prereq cache hit: restoring filepattern + pybind11
    xcopy /E /I /y prereq_cache\local_install C:\TEMP\argolid_bld\local_install
    REM Replicate the DLL copy that install_prereq_win.bat does for PATH
    if exist prereq_cache\local_install\bin (
        xcopy /E /I /y prereq_cache\local_install\bin %TEMP%\argolid\bin
    )
) else (
    echo =^> Prereq cache miss: building filepattern + pybind11 from scratch
    ci-utils\install_prereq_win.bat
    if errorlevel 1 exit 1
    xcopy /E /I /y local_install C:\TEMP\argolid_bld\local_install
)
