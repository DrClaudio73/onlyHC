#set(COMPONENT_SRCS "eventengine.c")
#set(COMPONENT_ADD_INCLUDEDIRS "./include")
#set(COMPONENT_REQUIRES "messagehandle manageUART auxiliaryfuncs")
#register_component()

idf_component_register(SRCS "eventengine.c"
                    INCLUDE_DIRS "./include"
                    PRIV_REQUIRES messagehandle manageUART typeconversion auxiliaryfuncs cripter)

