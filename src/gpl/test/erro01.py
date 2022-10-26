from openroad import Design, Tech
import helpers
import gpl_aux

tech = Tech()
tech.readLef("./nangate45.lef")
design = Design(tech)
design.readDef("./error01.def")

try:
    gpl_aux.global_placement(design, init_density_penalty=0.01,
                             skip_initial_place=True, density=0.001)
except Exception as inst:
    print(inst.args[0])


# source helpers.tcl
# set test_name error01 
# read_lef ./nangate45.lef
# read_def ./$test_name.def

# catch {global_placement -init_density_penalty 0.01 \
#          -skip_initial_place -density 0.001} error
# puts $error
