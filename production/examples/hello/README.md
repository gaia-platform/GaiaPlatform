# hello
This is the introductory example for Gaia Platform.

The code can be manually built and run using the commands from shell scripts `build.sh` and `run.sh`. In addition to these, `setup.sh` can be used to setup accessing the *hello* example data through Postgres, by creating a hello database and importing the Gaia tables into it.

To inspect the data, connect to Postgres as user *postgres*, then execute:

```
\c hello
select * from hello_fdw.names;
select * from hello_fdw.greetings;
```
