# Ping Pong

This is a simple example of high throughput application. The purpose is to test Gaia capabilities with high throughput
applications.

There are 2 actors, Main and Rules. Main initialize the application by writing "pong" in the `ping_pong.status` column.
Main then spawns one or more threads that read the value of `ping_pong.status`, and if the value is "pong" will write 
"ping". This will trigger the rules that will check if the value of `ping_pong.status` is "ping", and if so, it will
write "pong". This will repeat until the application is stopped.

A couple of considerations:
1. The Main workers will always check for "pong" and write "ping"
1. The Rules will always check for "ping" and write "pong"
1. The record updated by Main and Rules is always the same.

Note: to increase the chance of seeing problems increase the Main worker threads or the rules engine threads. Also, 
running the application in Debug, reduces the frequency of problems.
