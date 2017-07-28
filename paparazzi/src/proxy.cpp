#include <prime_server/prime_server.hpp>
#include <prime_server/http_protocol.hpp>
using namespace prime_server;
#include <prime_server/logging.hpp>

int main(int argc, char** argv) {
    if(argc < 3) {
        logging::ERROR("Usage: " + std::string(argv[0]) + " [tcp|ipc]://upstream_endpoint[:tcp_port] [tcp|ipc]://downstream_endpoint[:tcp_port]");
        return EXIT_FAILURE;
    }

    //TODO: validate these
    std::string upstream_endpoint(argv[1]);
    std::string downstream_endpoint(argv[2]);
    if(upstream_endpoint.find("://") != 3)
      logging::ERROR("bad upstream endpoint");
    if(downstream_endpoint.find("://") != 3)
      logging::ERROR("bad downstream endpoint");

    //start it up
    zmq::context_t context;
    proxy_t proxy(context, upstream_endpoint, downstream_endpoint,
        [](const std::list<zmq::message_t>& heart_beats,
           const std::list<zmq::message_t>& job) -> const zmq::message_t* {
            //parse the scene out
            auto request = http_request_t::from_string(static_cast<const char*>(job.front().data()), job.front().size());
            auto scene_itr = request.query.find("scene");
            std::string job_type;
            //its a POST of a custom style, we'll assume the size of the thing is unique enough
            if(scene_itr == request.query.cend() || scene_itr->second.size() == 0)
              job_type = std::to_string(request.body.size());
            //its in the request params
            else
              job_type = scene_itr->second.front();
            //have a look at each heartbeat
            for(const auto& heart_beat : heart_beats) {
                //the heartbeat is the scene
                std::string beat_type(static_cast<const char*>(heart_beat.data()));
                //did this worker work on the same scene as this job
                if(beat_type == job_type)
                    return &heart_beat;
            }
            //all of the heartbeats sucked so pick whichever
            return nullptr;
        }
    );

    //TODO: catch SIGINT for graceful shutdown
    proxy.forward();

    return EXIT_SUCCESS;
}
