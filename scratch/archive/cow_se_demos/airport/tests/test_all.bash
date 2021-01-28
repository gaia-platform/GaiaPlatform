printf "Running all tests:\n\n"

for i in test_*.py; do
    if [ $i = test_aids.py ];
        # Skip the nontest.
        then continue;
    else
        printf ">>> Running $i...\n"

        # Reset gaia store.
        python ../setup/gaia_init.py

        if ! python $i ;
           then echo; printf "******* $i test failed! *******\n"; exit 1;
        fi

        printf ">>> $i succeeded!\n\n"
    fi
done

printf "******* All tests succeeded! *******\n"
