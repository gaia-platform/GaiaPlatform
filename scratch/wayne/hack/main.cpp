#include "gaia_hack.h"
#include "gaia/system.hpp"
#include "gaia/rules/rules.hpp"
#include "hack_constants.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>
#include <fstream>
#include <unistd.h>

using namespace std;

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::direct_access;
using namespace gaia::hack;
using namespace gaia::rules;

// Verify or create administrative records.
void init_storage() {
    begin_transaction();
    auto id_association = id_association_t::get_first();
    if (!id_association)
    {
        id_association_writer ia;
        ia.insert_row();
    }
    commit_transaction();
}

void dump_db() {
    begin_transaction();
    printf("\nDatabase Contents\n");

    printf("-----------------------------------------\n");
    for (auto& p : Person_t::list())
    {
        printf("%s %s ID=%s: ", p.FirstName(), p.LastName(), p.PersonId());
        for (auto& s : p.Student_list())
        {
            printf(" STUDENT, ID=%s\n", s.StudentId());
        }
        for (auto& s : p.Staff_list())
        {
            printf(" STAFF, ID=%s\n", s.StaffId());
        }
        for (auto& pl : p.Parent_list())
        {
            printf(" PARENT, ID=%s\n", pl.ParentId());
        }
    }
    for (auto& f : Family_t::list())
    {
        printf("FAMILY, ID=%s\n", f.FamilyId());
        auto p = f.father_Parent();
        if (p)
        {
            auto person = p.Person();
            printf("  PARENT, ID=%s, %s, %s %s\n", p.ParentId(), p.Role(), person.FirstName(), person.LastName());
        }
        p = f.mother_Parent();
        if (p)
        {
            auto person = p.Person();
            printf("  PARENT, ID=%s, %s, %s %s\n", p.ParentId(), p.Role(), person.FirstName(), person.LastName());
        }
        auto s = f.Student();
        if (s)
        {
            auto person = s.Person();
            printf("  STUDENT, ID=%s, %s %s\n", s.StudentId(), person.FirstName(), person.LastName());
        }
    }
    for (auto& st : Staff_t::list())
    {
        auto person = st.Person();
        printf("STAFF, ID=%s, %s %s\n", st.StaffId(), person.FirstName(), person.LastName());
    }
    for (auto& e : Event_t::list())
    {
        printf("EVENT, ID=%s, %s, %s\n", e.EventId(), e.EventName(), e.EventDate());
        auto staff = e.staff_event_Staff();
        auto room = e.room_event_Room();
        printf("   STAFF: ID=%s\n", staff.StaffId());
        printf("   ROOM: ID=%s\n", room.RoomId());
    }
    for (auto& f : FaceScanLog_t::list())
    {
        printf("FACESCANLOG: ID=%s, %s %d\n", f.ScanLogId(), f.ScanDate(), f.ScanTime());
        auto building = f.building_scan_Building();
        if (building)
        {
            printf("   BUILDING: ID=%s, %s\n", building.BuildingId(), building.BuildingName());
        }
        auto person = f.person_scan_Person();
        if (person)
        {
            printf("   PERSON: ID=%s, %s %s\n", person.PersonId(), person.FirstName(), person.LastName());
        }
        auto stranger = f.stranger_scan_Stranger();
        if (stranger)
        {
            printf("   STRANGER: ID=%s, %d\n", stranger.StrangerId(), stranger.ScanCount());
        }
    }
    for (auto& s : Stranger_t::list())
    {
        printf("STRANGER: ID=%s, count=%d\n", s.StrangerId(), s.ScanCount());
        for (auto& f : s.stranger_scan_FaceScanLog_list())
        {
            printf("   FACESCANLOG: ID=%s, %s %d\n", f.ScanLogId(), f.ScanDate(), f.ScanTime());
        }
    }
    for (auto& e : Enrollment_t::list())
    {
        printf("ENROLLMENT, ID=%s, %s, %d\n", e.EnrollmentId(), e.EnrollmentDate(), e.EnrollmentTime());
        auto student = e.student_enrollment_Student();
        auto event = e.event_enrollment_Event();
        printf("   STUDENT: ID=%s\n", student.StudentId());
        printf("   EVENT: ID=%s\n", event.EventId());
    }
    printf("-----------------------------------------\n");
    printf("\n");
    commit_transaction();
}

// Scan for messages from the rules.
bool check_messages(string& message)
{
    bool saw_messages = false;

    begin_transaction();
    while (Message_t m = Message_t::get_first())
    {
        printf("Message [%s]\n", m.message_message());
        message = m.message_message();
        Message_Archive_writer ma;
        ma.archive_timestamp = m.message_timestamp();
        ma.archive_user = m.message_user();
        ma.archive_message = m.message_message();
        ma.insert_row();
        m.delete_row();
        saw_messages = true;
    }
    commit_transaction();

    return saw_messages;
}

string to_upper(string& word)
{
    std::transform(word.begin(), word.end(), word.begin(), ::toupper);
    return word;
}

void usage(const char*command) {
    printf("Usage: %s [batch_file_name]\n", command);
    printf(" batch_file_name: File containing People commands.\n");
    printf(" default: read from stdin\n");
}

class simulation_t
{
public:
    bool handle_main()
    {
        // Interactive (vs. batch) command composition.
        printf("\n");
        printf("%s / %s / %s / %s\n", c_register.c_str(), c_delete.c_str(), c_list.c_str(), c_quit.c_str());
        printf("enter command> ");

        get_upper(m_command);
        if (m_command == c_quit)
        {
            return false;
        }
        if (m_command != c_register && m_command != c_delete && m_command != c_list)
        {
            printf("Must be \"register\", \"delete\" or \"list\"\n");
            return true;
        }

        printf("\n");
        printf("%s / %s / %s", c_student.c_str(), c_staff.c_str(), c_parent.c_str());
        if (m_command == c_list)
        {
            printf(" / %s", c_family.c_str());
        }
        printf(" / %s\n", c_quit.c_str());
        printf("enter person-type> ");

        get_upper(m_person_type);

        if (m_command == c_quit)
        {
            return false;
        }

        if (m_person_type != c_student && m_person_type != c_staff && m_person_type != c_parent && m_person_type != c_family)
        {
            printf("Must be \"student\", \"staff\", \"parent\" or \"family\"\n");
            return true;
        }

        if (m_command == c_register)
        {
            if (m_person_type == c_student)
            {
                printf("enter first-name last-name birthdate> ");
            }
            else if (m_person_type == c_staff)
            {
                printf("enter first-name last-name birthdate hiredate> ");
            }
            else if (m_person_type == c_parent)
            {
                printf("enter first-name last-name birthdate student-ID father/mother> ");
            }
        }

        if (m_command == c_delete)
        {
            printf("%s-ID> ", m_person_type.c_str());
        }

        if (m_command == c_quit)
        {
            return false;
        }

        getline(cin, m_parameters);
        // Insert this command into the command log. This should trigger the appropriate action.
        Command_writer commands;
        commands.command_timestamp = time(NULL);
        commands.command_operation = m_command;
        commands.command_person_type = m_person_type;
        commands.command_parameters = m_parameters;
        begin_transaction();
        commands.insert_row();
        commit_transaction();
        return true;
    }

    bool is_equal(const string s1, const string s2)
    {
        return strcasecmp(s1.c_str(), s2.c_str()) == 0;
    }
    
    // Read next token from command-line.
    void get_upper(string& input)
    {
        getline(cin, input);
        to_upper(input);
    }

    int run() {
        string message;
        while (true) {
            if (!handle_main()) {
                return EXIT_SUCCESS;
            }
            check_messages(message);
        }
    }

private:
    string m_input;
    string m_command;
    string m_person_type;
    string m_parameters;
    Person_t m_current_Person;
    const char* m_current_Person_name;
    thread m_simulation_thread[1];
    int m_current_menu = 0;
};

constexpr char c_mem = '\\';

void show_list(string person_type)
{
    begin_transaction();
    if (person_type == c_person)
    {
        printf("PERSONS:\n");
        for (auto& s : gaia::hack::Person_t::list())
        {
            printf("%s: %s %s %d\n", s.PersonId(), s.FirstName(), s.LastName(), s.Birthdate());
        }
    }
    if (person_type == c_student)
    {
        printf("STUDENTS:\n");
        for (auto& s : gaia::hack::Student_t::list())
        {
            auto p = s.Person();
            printf("%s: %s %s %d\n", s.StudentId(), p.FirstName(), p.LastName(), p.Birthdate());
        }
    }
    if (person_type == c_staff)
    {
        printf("STAFF:\n");
        for (auto& s : gaia::hack::Staff_t::list())
        {
            auto p = s.Person();
            printf("%s: %s %s %d %s\n", s.StaffId(), p.FirstName(), p.LastName(), p.Birthdate(), s.HiredDate());
        }
    }
    if (person_type == c_parent)
    {
        printf("PARENTS:\n");
        for (auto& parent : gaia::hack::Parent_t::list())
        {
            auto person = parent.Person();
            printf("%s: %s %s %s\n", parent.ParentId(), person.FirstName(), person.LastName(), parent.Role());
            for (auto& f : parent.father_Family_list())
            {
                auto s = f.Student();
                auto person = s.Person();
                printf("   %s: %s %s\n", s.StudentId(), person.FirstName(), person.LastName());
            }
            for (auto& f : parent.mother_Family_list())
            {
                auto s = f.Student();
                auto person = s.Person();
                printf("   %s: %s %s\n", s.StudentId(), person.FirstName(), person.LastName());
            }
        }
    }
    if (person_type == c_family)
    {
        printf("FAMILIES:\n");
        for (auto& family : Family_t::list())
        {
            auto student = family.Student();
            auto mother = family.mother_Parent();
            auto father = family.father_Parent();
            auto p = student.Person();
            printf("%s: %s %s\n", student.StudentId(), p.FirstName(), p.LastName());
            if (mother)
            {
                p = mother.Person();
                printf("   %s: %s %s %s\n", mother.ParentId(), p.FirstName(), p.LastName(), mother.Role());
            }
            if (father)
            {
                p = father.Person();
                printf("   %s: %s %s %s\n", father.ParentId(), p.FirstName(), p.LastName(), father.Role());
            }
        }
    }
    if (person_type == c_building)
    {
        printf("BUILDINGS\n");
        for (auto& building : Building_t::list())
        {
            printf("%s: %s, Camera Id=%d\n", building.BuildingId(), building.BuildingName(), building.CameraId());
        }
    }
    if (person_type == c_room)
    {
        printf("ROOMS\n");
        for (auto& room : Room_t::list())
        {
            printf("%s: %s number=%d, floor=%d, capacity=%d\n", room.RoomId(), room.RoomName(),
                room.RoomNumber(), room.FloorNumber(), room.Capacity());
        }
    }
    if (person_type == c_event)
    {
        printf("EVENTS\n");
        for (auto& event : Event_t::list())
        {
            printf("%s: %s %s %d %d\n", event.EventId(), event.EventName(), event.EventDate(), event.EventStartTime(), event.EventEndTime());
        }
    }
    if (person_type == c_enroll)
    {
        printf("ENROLLMENT\n");
        for (auto& enroll : Enrollment_t::list())
        {
            printf("%s: %s %d\n", enroll.EnrollmentId(), enroll.EnrollmentDate(), enroll.EnrollmentTime());
        }
    }
    commit_transaction();
}

int main(int argc, const char**) {
    std::string server;
    string message;

    gaia::system::initialize("./gaia.conf");

    init_storage();

    if (argc > 0)
    {
        // Command Format:
        //   [memory-location] command person-type parameters
        //   where memory-location is optional word staring with '\'.
        //   a parameter that starts with '\' and matches the word
        //   will be replaced with the ID of the REGISTER result.
        string line;
        map<string, string> register_map;
        string command;
        string person_type;
        string param;
        string memory;
        while (getline(cin, line))
        {
            stringstream part(line);
            getline(part, command, ' ');

            // Check for comment - '#' as first character.
            if (command[0] == '#')
            {
                continue;
            }

            // Save memory name if present
            memory.clear();
            if (command[0] == c_mem)
            {
                memory = command;
                getline(part, command, ' ');
            }
            getline(part, person_type, ' ');
            // Perform substitutions on the parameters.
            string parameters;
            while (getline(part, param, ' '))
            {
                if (param[0] == c_mem)
                {
                    if (register_map.find(param) == register_map.end())
                    {
                        printf("cannot find substutution value for %s\n", param.c_str());
                        parameters += ' ' + param;
                    }
                    else
                    {
                        parameters += ' ' + register_map[param];
                    }
                }
                else
                {
                    parameters += ' ' + param;
                }
            }
            // There's always an extra space at the start that should be eliminated.
            parameters.erase(0,1);
            to_upper(command);
            to_upper(person_type);

            if (command == c_list)
            {
                show_list(person_type);
                continue;
            }

            if (command == c_quit)
            {
                break;
            }

            if (command == c_dump)
            {
                dump_db();
                continue;
            }

            // Insert this command into the command log. This should trigger the appropriate action.
            Command_writer commands;
            commands.command_timestamp = time(NULL);
            commands.command_operation = command;
            commands.command_person_type = person_type;
            commands.command_parameters = parameters;
            printf("%s %s %s\n", command.c_str(), person_type.c_str(), parameters.c_str());
            begin_transaction();
            commands.insert_row();
            commit_transaction();

            // Read/print/delete messages from the rules.
            int delay = 0;
            while (!check_messages(message))
            {
                usleep(1000);
                // Prevent infinite loops when no message is sent.
                if (++delay > 10)
                {
                    printf("Timed out waiting for rule message. Bad command?\n");
                    break;
                }
            }
            if (memory.size())
            {
                register_map[memory] = message;
            }
        }
    }
    else 
    {
        simulation_t sim;
        sim.run();
    }

    int delay = 0;
    while (!check_messages(message))
    {
        usleep(1000);
        if (++delay > 10)
        {
            break;
        }
    }

    // dump_db();
    gaia::system::shutdown();
}
