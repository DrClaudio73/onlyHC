#set(COMPONENT_SRCS "messagehandle.c")
#set(COMPONENT_ADD_INCLUDEDIRS "./include")
#set(COMPONENT_REQUIRES "typeconversion")
#register_component()

idf_component_register(SRCS "messagehandle.c"
                    INCLUDE_DIRS "./include"
                    PRIV_REQUIRES typeconversion)