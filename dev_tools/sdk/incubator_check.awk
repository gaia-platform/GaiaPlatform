{
    # Input expects the values of the following columns:
    #
    # 1:sched 2:invoc 3:pend 4:aband 5:retry 6:excep
    # Example: 32 32 0 0 0 0
    #
    # These are extracted from the gaia_stats_log file in three steps:
    # 1) tail: get the last data row written to the stats file.
    # 2) tr: sqeeze all the spaces into a single space (compress the columns)
    # 3) cut: extract the 6 columns above where the delimeter between columns is a space.
    # Example:  tail -n 1 gaia_stats.log | tr -s ' ' ' ' | cut -d ' ' -f 8-13
    #
    # Check the following:
    # 1) At least one rule was scheduled (Example: 32)
    # 2) The number of rules scheduled was equal to the number of rules invoked
    # 3) No rules were pending, abandoned, retried, or threw an exception.
    #
    # If all of these checks succeed, then print SUCCESS. Otherwise, print FAILED.
    #
    if ($1 != 0 && $2 == $1 && $3 == 0 && $4 == 0 && $5 == 0 && $6 == 0 ) 
        printf "SUCCESS\n"
    else
        printf "FAILED\n"
}