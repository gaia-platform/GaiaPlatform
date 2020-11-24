#pragma once

/**
* @brief The message bus interface
*/

class ICampus
{
protected: 

    //inline static const std::shared_ptr<ICampus> campus_ruleset_p_campus = nullptr;
    inline static ICampus* campus_ruleset_p_campus = nullptr;

public:

    static ICampus* get_callback_class(){
        return campus_ruleset_p_campus;
    }

    /*static std::shared_ptr<ICampus> get_callback_classy(){
        return std::make_shared<ICampus>(campus_ruleset_p_campus);
    }*/

    virtual void cb_action( std::string actorType, 
        std::string actorName, std::string actionName, std::string arg1) = 0;    
        
    virtual void cb_alert( std::string title, 
        std::string body, int severity, std::string arg1) = 0;

    ICampus(){}
  
    // to get rid of annoying build warnings
    virtual ~ICampus(){}
    
    int DemoTest(){
        return 0;
    }
};