import utl

# Definitions
DEFAULT_TARGET_DENSITY = 0.7


# Besides design, there are no positional args here. General strategy is that
# when an arg is None, we just skip setting it, otherwise we will set the
# parameter after a quick type check.
def tritonPartDesign(
    design,
    *,
    base_balance=[1.0],
    scale_factor=[1.0],
    num_parts=2,
    balance_constraint=1.0,
    seed=1,
    timing_aware_flag=True,
    top_n=1000,
    placement_flag=False,
    fence_flag=False,
    fence_lx=0.0,
    fence_ly=0.0,
    fence_ux=0.0,
    fence_uy=0.0,
    fixed_file="",
    community_file="",
    group_file="",
    solution_file="",
    net_timing_factor=1.0,
    path_timing_factor=1.0,
    path_snaking_factor=1.0,
    timing_exp_factor=1.0,
    extra_delay=1e-9,
    guardband_flag=False,
    e_wt_factors=[1.0],
    v_wt_factors=[1.0],
    placement_wt_factors=[1.0],
    thr_coarsen_hyperedge_size_skip=1000,
    thr_coarsen_vertices=10,
    thr_coarsen_hyperedges=50,
    coarsening_ratio=1.5,
    max_coarsen_iters=30,
    adj_diff_ratio=0.0001,
    min_num_vertices_each_part=4,
    num_initial_solutions=100,
    num_best_initial_solutions=10,
    refiner_iters=10,
    max_moves=100,
    early_stop_ratio=0.5,
    total_corking_passes=25,
    v_cycle_flag=True,
    max_num_vcycle=1,
    num_coarsen_solutions=4,
    num_vertices_threshold_ilp=50,
    global_net_threshold=1000
):
    mgr = design.getPartitionMgr()

    mgr.tritonPartDesign(
        num_parts,
        balance_constraint,
        base_balance,
        scale_factor,
        seed,
        timing_aware_flag,
        top_n,
        placement_flag,
        fence_flag,
        fence_lx,
        fence_ly,
        fence_ux,
        fence_uy,
        fixed_file,
        community_file,
        group_file,
        solution_file,
        net_timing_factor,
        path_timing_factor,
        path_snaking_factor,
        timing_exp_factor,
        extra_delay,
        guardband_flag,
        e_wt_factors,
        v_wt_factors,
        placement_wt_factors,
        thr_coarsen_hyperedge_size_skip,
        thr_coarsen_vertices,
        thr_coarsen_hyperedges,
        coarsening_ratio,
        max_coarsen_iters,
        adj_diff_ratio,
        min_num_vertices_each_part,
        num_initial_solutions,
        num_best_initial_solutions,
        refiner_iters,
        max_moves,
        early_stop_ratio,
        total_corking_passes,
        v_cycle_flag,
        max_num_vcycle,
        num_coarsen_solutions,
        num_vertices_threshold_ilp,
        global_net_threshold,
    )
