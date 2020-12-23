#pragma once

/**
* @brief Interface used to expose campus core methods inside a Gaia ruleset rules
*/

class i_Campus
{
protected: 

// 1) We ignore -Wc++17-extensions here to quiet down the Gaia translator
// 2) We mention GCC but this works for Clang as well
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++17-extensions"
    //inline static const std::shared_ptr<i_Campus> campus_ruleset_p_campus = nullptr;

    // The pointer to the live instance of the campus core class
    // TODO : This is not an example to good coding practice, need to make safe and modern
    inline static i_Campus* campus_ruleset_p_campus = nullptr;
#pragma GCC diagnostic pop

public:

    /**
     * Class factory, this is the only method allowed for obtaining an instance of the 
     * class withing a ruleset rule
     * 
     * @return i_Campus*
     * @throws 
     * @exceptsafe yes
     */   
    static i_Campus* get_callback_class(){
        return campus_ruleset_p_campus;
    }

    /*static std::shared_ptr<i_Campus> get_callback_classy(){
        return std::make_shared<i_Campus>(campus_ruleset_p_campus);
    }*/

    /**
     * Call this from within a ruleset rule to send an action message
     * 
     * @param[in] std::string actorType
     * @param[in] std::string actorName
     * @param[in] std::string actionName
     * @param[in] std::string arg1
     * @return void
     * @throws 
     * @exceptsafe yes
     */   
    virtual void cb_action( std::string actorType, 
        std::string actorName, std::string actionName, std::string arg1) = 0;    
        
    /**
     * Call this from within a ruleset rule to send an alert message
     * 
     * @param[in] std::string title
     * @param[in] std::string body
     * @param[in] int severity
     * @param[in] std::string arg1
     * @return void
     * @throws 
     * @exceptsafe yes
     */   
    virtual void cb_alert( std::string title, 
        std::string body, int severity, std::string arg1) = 0;

    /**
     * Call this from within a ruleset rule to find a new room for an event
     * 
     * @param[in] std::string eventName
     * @return std::string
     * @throws 
     * @exceptsafe yes
     */   
    virtual std::string cb_find_new_event_room( std::string eventName) = 0;

    /**
     * Constructor
     * 
     * @throws 
     * @exceptsafe yes
     */   
    i_Campus(){}
  
    /**
     * Destructor, to get rid of annoying build warnings
     * 
     * @throws 
     * @exceptsafe yes
     */   
    virtual ~i_Campus(){}
    
    /**
     * Unit test, needs to be implemented
     * 
     * @throws 
     * @exceptsafe yes
     */       
    int demo_test(){
        return 0;
    }
};