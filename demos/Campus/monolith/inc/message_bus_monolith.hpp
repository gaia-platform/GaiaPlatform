#pragma once

#include "../../common/inc/message_bus.hpp"

namespace message
{
/**
 * @brief A local in process message bus
 */
class message_bus : public i_message_bus
{
private:
    std::thread* m_workerThread;
    std::queue<std::shared_ptr<bus_messages::message>> m_messageQueue;
    std::mutex m_mut;
    std::condition_variable m_cv;
    bool m_stop = false;

    /**
     * Log message to stdout
     * 
     * @param[in] std::string prefix e
     * @param[in] const std::exception& e
     * @return void
     * @throws 
     * @exceptsafe yes
     */
    void log_this(std::string prefix, const std::exception& e)
    {
        std::cout << "Exception: " << prefix << " : " << e.what();
    }

    /**
     * Log message to stdout
     * 
     * @param[in] std::string prefix e
     * @return void
     * @throws 
     * @exceptsafe yes
     */
    void log_this(std::string prefix)
    {
        std::cout << prefix;
    }

    /**
     * Check if message sender is the callback registrant
     * 
     * @param[in] std::shared_ptr<bus_messages::message> msg
     * @param[in] callback_registration registration
     * @return true if the same, false if not the same
     * @throws std::invalid_argument
     * @exceptsafe yes
     */
    bool is_registrant_sender(std::shared_ptr<bus_messages::message> msg, callback_registration registration)
    {
        if (nullptr == msg)
            throw std::invalid_argument("argument msg cannot be null");

        //auto registrant_name = registration.get_registrant_name();
        //auto sender_name = msg->get_sender_name();

        return 0 == registration.get_registrant_name().compare(msg->get_sender_name()) ? true : false;
    }

    /**
     * Worker thread, runs forever
     *
     * @return 
     * @throws 
     * @exceptsafe yes
     */
    void worker()
    {
        while (!m_stop)
        {
            // Wait until a message is received
            std::unique_lock<std::mutex> lk(m_mut);
            m_cv.wait(lk);

            if (m_stop)
                break;

            // empty the queue
            while (!m_messageQueue.empty())
            {
                // get the first message
                std::shared_ptr<bus_messages::message> msg = m_messageQueue.front();
                m_messageQueue.pop();

                if (nullptr == msg)
                {
                    log_this("messge == null");
                    continue;
                }

                // send message to each registered callback
                for (callback_registration cbr : m_messageCallbacks)
                {
                    try
                    {
                        // Don't echo messages back to sender
                        if (!is_registrant_sender(msg, cbr))
                            cbr.get_callback()(msg);
                    }
                    catch (const std::exception& e)
                    {
                        log_this("Processing callback", e);
                    }
                }
            }
        }
    }

public:
    /**
     * constructor
     *
     * @return 
     * @throws 
     * @exceptsafe yes
     */
    message_bus()
    {
        run();
    }

    /**
     * Put a message on the bus
     *
     * @param[in] std::shared_ptr<bus_messages::message> msg
     * @return int
     * @throws 
     * @exceptsafe yes
     */
    int send_message(std::shared_ptr<bus_messages::message> msg) override
    {
        {
            //std::lock_guard<std::mutex> lk(m);
            m_messageQueue.push(msg);
        }

        m_cv.notify_one();
        return 0;
    }

    /**
     * Start the worker thread
     *
     * @return int
     * @throws 
     * @exceptsafe yes
     */
    int run()
    {
        m_workerThread = new std::thread(&message_bus::worker, this);
        return 0;
    }

    /**
     * Stop the worker thread
     *
     * @throws 
     * @exceptsafe yes
     */
    void stop()
    {
        m_stop = true;

        {
            std::lock_guard<std::mutex> lk(m_mut);
        }

        m_cv.notify_one();
    }
};

} // namespace message