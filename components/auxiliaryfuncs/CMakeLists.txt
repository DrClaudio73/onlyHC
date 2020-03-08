#set(COMPONENT_SRCS "auxiliaryfuncs.c")
#set(COMPONENT_ADD_INCLUDEDIRS "./include")
#register_component()

idf_component_register(SRCS "auxiliaryfuncs.c"
                    INCLUDE_DIRS "./include")