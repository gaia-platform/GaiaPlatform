@echo off
SET ADLINK_DATARIVER_URI=file://%EDGE_SDK_HOME%/etc/config/default_datariver_config_v1.6.xml
SET THING_PROPERTIES_URI=file://./config/TemperatureDisplayProperties.json
SET RUNNING_TIME=%1
IF "%1"=="" (
    SET RUNNING_TIME=60
)

SET EXEC_CMD=temperaturedisplay.exe %THING_PROPERTIES_URI% %RUNNING_TIME%

IF "%2"=="-fg" (
    %EXEC_CMD%
) ELSE (
    start "Temperature Display" cmd /K "%EXEC_CMD%"
)