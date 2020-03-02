#idf_component_register(SRCS "main.c" INCLUDE_DIRS "./include")
idf_component_register(SRCS "mainhttpd.c"
                    INCLUDE_DIRS "./include"
                    REQUIRES esp_http_server)
#set(COMPONENT_SRCS "mainhttpd.c")
#set(COMPONENT_ADD_INCLUDEDIRS "./include")

#register_component()