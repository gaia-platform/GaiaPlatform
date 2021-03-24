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
    Grade string,
    registered_student references Student,
    registered_course references Course
);

create table PreReq (
    PreReqId string,
    MinGrade string,
    prereq references Course,
    course references Course
);

