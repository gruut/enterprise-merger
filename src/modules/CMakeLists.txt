add_library(modules
        module.hpp
        message_fetcher/message_fetcher.hpp
        communication/communication.hpp
        )
set_target_properties(modules PROPERTIES LINKER_LANGUAGE CXX)
