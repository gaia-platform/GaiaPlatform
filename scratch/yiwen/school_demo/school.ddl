CREATE TABLE IF NOT EXISTS sim (
  now : int64 active,
  NextDay: int64 active,

  MyFace: int64,
  MyName: string
);

CREATE TABLE IF NOT EXISTS msg (
  Number : string active,
  Text : string active
);

CREATE TABLE IF NOT EXISTS person (
  FirstName : string,
  LastName : string,
  BirthDate : int64,
  FaceSignature : uint64
);

CREATE TABLE IF NOT EXISTS staff(
  HiredDate : int64,
  StaffAsPerson references person
);

CREATE TABLE IF NOT EXISTS parent(
  MotherFather : bool,
  ParentAsPerson references person
);

CREATE TABLE IF NOT EXISTS student(
  Number : string,
  StudentAsPerson references person,
  Father references parent,
  Mother references parent
);

CREATE TABLE IF NOT EXISTS building(
  BuildingName : string,
  CameraId : uint64,
  DoorClosed : bool
);

CREATE TABLE IF NOT EXISTS room (
  RoomNumber : string,
  RoomName : string,
  FloorNumber : int32,
  Capacity : uint32,
  InBuilding references building
);

CREATE TABLE IF NOT EXISTS FaceScanLog(
  ScanSignature: uint64 active,
  ScanTime : int64,
  Building references building
);

CREATE TABLE IF NOT EXISTS stranger (
  FaceScanCount: uint32,
  FirstScanned references FaceScanLog
);

CREATE TABLE IF NOT EXISTS event (
  Name : string,
  Version : uint32 active,
  StartTime : int64,
  EndTime : int64,
  EventActualEnrolled : uint32,
  Enrollment : uint32 active,
  EventRoom references room,
  Teacher references staff
);

CREATE TABLE IF NOT EXISTS registration (
  RegistrationTime : int64,
  Event references event,
  Student references student
);

CREATE TABLE IF NOT EXISTS CheckConflictTask (
  trivial : bool,
  IsConflict references registration
);
