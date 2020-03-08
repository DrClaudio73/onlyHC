#set(COMPONENT_SRCS "manageUART.c")
#set(COMPONENT_ADD_INCLUDEDIRS "./include")
#register_component()

idf_component_register(SRCS "manageUART.c"
                    INCLUDE_DIRS "./include")


