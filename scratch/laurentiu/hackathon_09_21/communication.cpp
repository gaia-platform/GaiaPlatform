/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "communication.hpp"

#include <chrono>

#include <iostream>
#include <limits>
#include <vector>

#include <aws/crt/Api.h>
#include <aws/crt/StlAllocator.h>
#include <aws/crt/UUID.h>
#include <aws/crt/auth/Credentials.h>
#include <aws/iot/MqttClient.h>

#include "gaia/logger.hpp"
#include "gaia/system.hpp"

using namespace Aws::Crt;
using namespace std;
using namespace gaia::amr_swarm;



namespace gaia
{
namespace amr_swarm
{
namespace communication
{

std::shared_ptr<Aws::Crt::Mqtt::MqttConnection> g_connection;

string g_endpoint;
string g_region;
string g_remote_client_id;

string get_uuid()
{
    return Aws::Crt::UUID().ToString().c_str();
}

void publish_message(const string& topic, const string& payload)
{
    auto on_publish_complete = [](Mqtt::MqttConnection&, uint16_t packet_id, int error_code)
    {
        if (packet_id)
        {
            gaia_log::app().trace("Operation on packet_id {} Succeeded", packet_id);
        }
        else
        {
            gaia_log::app().error("Operation failed with error {}", aws_error_debug_str(error_code));
        }
    };

    if (g_connection)
    {
        ByteBuf payload_buf = ByteBufFromArray(reinterpret_cast<const uint8_t*>(payload.c_str()), payload.length());
        string full_topic = g_remote_client_id + "/" + topic;
        gaia_log::app().info("Publishing on topic:{} payload:{}", full_topic, payload);
        g_connection->Publish(full_topic.c_str(), AWS_MQTT_QOS_AT_LEAST_ONCE, false, payload_buf, on_publish_complete);
    }
}

void print_help()
{
    fprintf(stdout, "Usage:\n");
    fprintf(stdout, "amr_swarm --endpoint <endpoint> --region <region> --gateway-id <gateway-id>\n\n");
    fprintf(stdout, "endpoint: the endpoint of the mqtt server not including a port\n");
    fprintf(stdout, "region: aws region (e.g. us-west-2)\n");
    fprintf(stdout, "remote-client-id: mqtt client id of simulator or other publisher/subscriber of messages\n");
}

void print_aws_creds_error()
{
    gaia_log::app().error(
        "Error: Missing AWS credentials. Must set environment variables"
        " AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY, and AWS_SESSION_TOKEN");
}

bool cmd_option_exists(char** begin, char** end, const String& option)
{
    return find(begin, end, option) != end;
}

char* get_cmd_option(char** begin, char** end, const String& option)
{
    char** itr = find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool init(int argc, char* argv[])
{
    /*********************** Parse Arguments ***************************/
    if (!cmd_option_exists(argv, argv + argc, "--endpoint")
        || !cmd_option_exists(argv, argv + argc, "--region")
        || !cmd_option_exists(argv, argv + argc, "--remote-client-id"))
    {
        print_help();
        return false;
    }

    if (!getenv("AWS_ACCESS_KEY_ID") || !getenv("AWS_SECRET_ACCESS_KEY")
        || !getenv("AWS_SESSION_TOKEN"))
    {
        print_aws_creds_error();
        return false;
    }

    g_endpoint = get_cmd_option(argv, argv + argc, "--endpoint");
    g_region = get_cmd_option(argv, argv + argc, "--region");
    g_remote_client_id = get_cmd_option(argv, argv + argc, "--remote-client-id");

    return true;
}

void connect(message_callback_t callback)
{
    string client_id = get_uuid();
    string subscribe_topic = client_id + "/#";

    ApiHandle api_handle;

    Io::EventLoopGroup event_loop_group(1);
    if (!event_loop_group)
    {
        gaia_log::app().error("Event Loop Group Creation failed with error {}", ErrorDebugString(event_loop_group.LastError()));
        exit(-1);
    }

    Aws::Crt::Io::DefaultHostResolver default_host_resolver(event_loop_group, 1, 5);
    Io::ClientBootstrap bootstrap(event_loop_group, default_host_resolver);

    if (!bootstrap)
    {
        gaia_log::app().error("ClientBootstrap failed with error {}", ErrorDebugString(bootstrap.LastError()));
        exit(-1);
    }

    std::shared_ptr<Aws::Crt::Auth::ICredentialsProvider> provider = nullptr;

    Aws::Crt::Auth::CredentialsProviderStaticConfig config;
    config.AccessKeyId = Aws::Crt::ByteCursorFromCString(getenv("AWS_ACCESS_KEY_ID"));
    config.SecretAccessKey = Aws::Crt::ByteCursorFromCString(getenv("AWS_SECRET_ACCESS_KEY"));
    config.SessionToken = Aws::Crt::ByteCursorFromCString(getenv("AWS_SESSION_TOKEN"));

    provider = Aws::Crt::Auth::CredentialsProvider::CreateCredentialsProviderStatic(config);

    Aws::Iot::MqttClientConnectionConfigBuilder builder;
    Aws::Iot::WebsocketConfig ws_config(g_region.c_str(), provider);
    builder = Aws::Iot::MqttClientConnectionConfigBuilder(ws_config);
    builder.WithEndpoint(g_endpoint.c_str());

    auto client_config = builder.Build();
    if (!client_config)
    {
        gaia_log::app().error("Client Configuration initialization failed with error {}", ErrorDebugString(client_config.LastError()));
        exit(-1);
    }

    Aws::Iot::MqttClient mqtt_client(bootstrap);

    if (!mqtt_client)
    {
        gaia_log::app().error("MQTT Client Creation failed with error {}", ErrorDebugString(mqtt_client.LastError()));
        exit(-1);
    }

    g_connection = mqtt_client.NewConnection(client_config);

    if (!g_connection)
    {
        gaia_log::app().error("MQTT Connection Creation failed with error {}", ErrorDebugString(mqtt_client.LastError()));
        exit(-1);
    }

    std::promise<bool> connection_completed_promise;
    std::promise<void> connection_closed_promise;

    auto on_connection_completed = [&](Mqtt::MqttConnection&, int error_code, Mqtt::ReturnCode return_code, bool)
    {
        if (error_code)
        {
            gaia_log::app().error("Connection failed with error {}", ErrorDebugString(error_code));
            connection_completed_promise.set_value(false);
        }
        else
        {
            if (return_code != AWS_MQTT_CONNECT_ACCEPTED)
            {
                gaia_log::app().error("Connection failed with mqtt return code {}", (int)return_code);
                connection_completed_promise.set_value(false);
            }
            else
            {
                gaia_log::app().info("Connection completed successfully.");
                gaia::system::initialize();
                connection_completed_promise.set_value(true);
            }
        }
    };

    auto on_interrupted = [&](Mqtt::MqttConnection&, int error)
    {
        gaia_log::app().error("Connection interrupted with error {}", ErrorDebugString(error));
    };

    auto on_resumed = [&](Mqtt::MqttConnection&, Mqtt::ReturnCode, bool)
    { gaia_log::app().info("Connection resumed"); };

    auto on_disconnect = [&](Mqtt::MqttConnection&)
    {
        {
            gaia_log::app().info("Disconnect completed");
            gaia::system::shutdown();
            connection_closed_promise.set_value();
        }
    };

    auto on_message = [&](Mqtt::MqttConnection&, const String& topic, const ByteBuf& payload,
                          bool /*dup*/, Mqtt::QOS /*qos*/, bool /*retain*/)
    {
        callback(string(topic.c_str()), string(reinterpret_cast<char*>(payload.buffer), payload.len));
    };

    g_connection->OnConnectionCompleted = std::move(on_connection_completed);
    g_connection->OnDisconnect = std::move(on_disconnect);
    g_connection->OnConnectionInterrupted = std::move(on_interrupted);
    g_connection->OnConnectionResumed = std::move(on_resumed);

    gaia_log::app().info("Connecting...");
    if (!g_connection->Connect(client_id.c_str(), false, 1000))
    {
        gaia_log::app().error("MQTT Connection failed with error {}", ErrorDebugString(g_connection->LastError()));
        exit(-1);
    }

    if (connection_completed_promise.get_future().get())
    {
        std::promise<void> subscribe_finished_promise;
        auto on_sub_ack =
            [&](Mqtt::MqttConnection&, uint16_t packet_id, const String& topic, Mqtt::QOS qos, int error_code)
        {
            if (error_code)
            {
                gaia_log::app().error("Subscribe failed with error {}", aws_error_debug_str(error_code));
                exit(-1);
            }
            else
            {
                if (!packet_id || qos == AWS_MQTT_QOS_FAILURE)
                {
                    gaia_log::app().error("Subscribe rejected by the broker.");
                    exit(-1);
                }
                else
                {
                    gaia_log::app().info("Subscribe on topic {} on packet_id {} Succeeded", topic.c_str(), packet_id);
                }
            }
            subscribe_finished_promise.set_value();
        };

        g_connection->Subscribe(subscribe_topic.c_str(), AWS_MQTT_QOS_AT_LEAST_ONCE, on_message, on_sub_ack);
        subscribe_finished_promise.get_future().wait();
        publish_message("appUUID", client_id);

        String input;
        gaia_log::app().info("Press 'enter' to exit this program.");
        gaia_log::app().info("Waiting to connect to the Sandbox...");
        std::getline(std::cin, input);

        std::promise<void> unsubscribe_finished_promise;
        g_connection->Unsubscribe(
            "#", [&](Mqtt::MqttConnection&, uint16_t, int)
            { unsubscribe_finished_promise.set_value(); });
        unsubscribe_finished_promise.get_future().wait();
    }

    if (g_connection->Disconnect())
    {
        connection_closed_promise.get_future().wait();
    }
}

} // namespace communication
} // namespace amr_swarm
} // namespace gaia
