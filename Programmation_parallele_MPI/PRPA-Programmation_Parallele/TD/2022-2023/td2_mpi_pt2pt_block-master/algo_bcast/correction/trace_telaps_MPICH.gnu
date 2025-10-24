set grid
set logscale
#plot "telaps_bcast_48_MPICH" w lp, "telaps_linear_48_MPICH" w lp, "telaps_btreev1_48_MPICH" w lp, "telaps_btreev2_48_MPICH" w lp, "telaps_btopo_48_MPICH" w lp
plot "telaps_allalgos_48_MPICH" u 1:2 w lp t "bcast", "" u 1:3 w lp t "linear", "" u 1:4 w lp t "btreev1", "" u 1:5 w lp t "btreev2", "" u 1:6 w lp t "btopo"

