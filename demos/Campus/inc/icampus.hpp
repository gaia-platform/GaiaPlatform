#pragma once

/**
* @brief Interface used to expose campus core methods inside a Gaia ruleset rules
*/

class ICampus
{
protected: 

// 1) We ignore -Wc++17-extensions here to quiet down the Gaia translator
// 2) We mention GCC but this works for Clang as well
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++17-extensions"
    //inline static const std::shared_ptr<ICampus> campus_ruleset_p_campus = nullptr;

    // The pointer to the live instance of the campus core class
    // TODO : This is not an example to good coding practice, need to make safe and modern
    inline static ICampus* campus_ruleset_p_campus = nullptr;
#pragma GCC diagnostic pop

public:

    /**
     * Class factory, this is the only method allowed for obtaining an instance of the 
     * class withing a ruleset rule
     * 
     * @return ICampus*
     * @throws 
     * @exceptsafe yes
     */   
    static ICampus* get_callback_class(){
        return campus_ruleset_p_campus;
    }

    /*static std::shared_ptr<ICampus> get_callback_classy(){
        return std::make_shared<ICampus>(campus_ruleset_p_campus);
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
     * Constructor
     * 
     * @throws 
     * @exceptsafe yes
     */   
    ICampus(){}
  
    /**
     * Destructor, to get rid of annoying build warnings
     * 
     * @throws 
     * @exceptsafe yes
     */   
    virtual ~ICampus(){}
    
    /**
     * Unit test, needs to be implemented
     * 
     * @throws 
     * @exceptsafe yes
     */       
    int DemoTest(){
        return 0;
    }
};