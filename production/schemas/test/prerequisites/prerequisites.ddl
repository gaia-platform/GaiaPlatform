create table Student (
    StudentId string,
    Surname string,
    Age int32,
    TotalHours int32,
    GPA float
);

create table Course (
    CourseId string,
    Name string,
    Hours int32
);

create table Registration (
    RegId string,
    Status string,
    Grade string
);

create relationship if not exists StudentReg (
    Student.registrations -> Registration[],
    Registration.registered_student -> Student
);

create relationship if not exists CourseReg (
    Course.registrations -> Registration[],
    Registration.registered_course -> Course
);

create table PreReq (
    PreReqId string,
    MinGrade string
);

create relationship if not exists PreReqCourses (
    PreReq.prereq -> Course,
    Course.required_by -> PreReq
);

create relationship if not exists PreReqRef (
    PreReq.course -> Course,
    Course.requires -> PreReq
);
