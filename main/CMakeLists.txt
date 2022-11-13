idf_component_register(SRCS "router_main.c" "routines.c"
                    INCLUDE_DIRS "."
		    REQUIRES soc sensor util event_handler)
set(ulp_app_name ulp_${COMPONENT_NAME})
set(ulp_s_sources ulp/ulp_main.S)
set(ulp_exp_dep_srcs "router_main.c")

ulp_embed_binary(${ulp_app_name} "${ulp_s_sources}" "${ulp_exp_dep_srcs}")
