// DDL with comments. This cannot be compiled by gaiac as-is.
// The Cmake will preprocess this into hack.ddl, which has the comments removed.

// One Person can be Staff, Student, Parent or Stranger.
create table Person (
    PersonId string,
    FirstName string active,
    LastName string,
    Birthdate int32,
    FaceSignature string
);
// Vendors, Teachers, local workers, etc.
create table Staff (
    StaffId string,
    HiredDate string,
    references Person
);
// Able to enroll in Event.
create table Student (
    StudentId string active,
    Number uint64,
    references Person
);
// Either Father or Mother (Role).
// Can be connected to multiple Family records (one for each student).
create table Parent (
    ParentId string active,
    Role string,
    references Person
);
// Owned by up to 3 people, the Student, Father (Parent) and Mother (Parent).
create table Family (
    FamilyId string,
    Student_Family string,
    Father_Family string,
    Mother_Family string,
    references Student,
    father references Parent,
    mother references Parent
);

create table Building (
    BuildingId string,
    BuildingName string,
    CameraId int16,
    DoorClosed bool
);
// Person found through a face scan, but not a known Person.
create table Stranger (
    StrangerId string,
    ScanCount int16 active
);
create table FaceScanLog (
    ScanLogId string,
    Building_ScanLog string,
    Person_ScanLog string,
    ScanSignature string,
    ScanSignatureId string,  // References a Person? Not used.
    ScanDate string,
    ScanTime int32,
    references Person,
    references Building,
    references Stranger
);
create table Room (
    RoomId string,
    Building_Room string,
    RoomNumber int32,
    RoomName string,
    FloorNumber int16,
    Capacity int16,
    references Building
);
// Represents a class or seminar.
create table Event (
    EventId string,
    Staff_Event string,
    Room_Event string,
    EventName string,
    EventDate string,
    EventStartTime int32,
    EventEndTime int32,
    EventActualEnrolled bool,
    EventEnrollment int16,
    staff_event references Staff,
    room_event references Room
);
create table Enrollment (
    EnrollmentId string,
    Student_Enrollment string,
    Event_Enrollment string,
    EnrollmentDate string,
    EnrollmentTime int32,
    student_enrollment references Student,
    event_enrollment references Event
);

// Administration Tables

// Log of Person registrations
create table Register_Command (
    register_timestamp int32,
    register_person_type string,
    register_parameters string
);
// Log of Person deletions
create table Delete_Command (
    delete_timestamp int32,
    delete_person_type string,
    delete_id string
);
// Command table is log from front-end, read by rules.
create table Command (
    command_timestamp int32,
    command_operation string,
    command_person_type string,
    command_parameters string
);
create table Processed_Command (
    processed_timestamp int32,
    processed_time_done int32,
    processed_operation string,
    processed_person_type string,
    processed_parameters string
);
// id_associations are used to assign unique ID's to Person, Staff, Student, Parent and Families.
create table id_association (
    person int32,
    staff int32,
    student int32,
    parent int32,
    family int32,
    building int32,
    room int32,
    scan int32,
    event int32,
    enrollment int32,
    student_number int32  // This may correspond to StudentID, so redundant.
);
// Message table is log from rules, read & archived by front-end.
create table Message (
    message_timestamp int32,
    message_user string,  // User is operating app, which may be multi-user.
    message_message string
);
create table Message_Archive (
    archive_timestamp int32,
    archive_user string,  // User is operating app, which may be multi-user.
    archive_message string
);
