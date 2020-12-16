---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

DROP DATABASE IF EXISTS campus;

CREATE DATABASE campus;

\c campus;

CREATE EXTENSION gaia_fdw;

CREATE SERVER gaia FOREIGN DATA WRAPPER gaia_fdw;

CREATE SCHEMA campus_fdw;

IMPORT FOREIGN SCHEMA campus_fdw
    FROM SERVER gaia INTO campus_fdw;

-- Insert some data. Start with staff, guardians, and students.

-- Staff - Principal.
INSERT INTO campus_fdw.persons (first_name, last_name, birth_date, contact_info, face_signature)
    VALUES ('George', 'Phearsons', '1965-05-06', 'geoph@aol.com', 'GPH000');

INSERT INTO campus_fdw.staff (hiring_date, title)
    VALUES ('08/26/1997', 'Principal');

UPDATE campus_fdw.staff
    SET (staff_person) = (
        SELECT gaia_id FROM campus_fdw.persons WHERE face_signature = 'GPH000')
    WHERE staff_person is null;

-- Staff - PE Instructor.
INSERT INTO campus_fdw.persons (first_name, last_name, birth_date, contact_info, face_signature)
    VALUES ('Jill', 'Valentine', '1977-08-20', 'jill77@gmail.com', 'JVA001');

INSERT INTO campus_fdw.staff (hiring_date, title)
    VALUES ('07/12/2009', 'PE Instructor');

UPDATE campus_fdw.staff
    SET (staff_person) = (
        SELECT gaia_id FROM campus_fdw.persons WHERE face_signature = 'JVA001')
    WHERE staff_person is null;

-- Staff - Social Studies Teacher.
INSERT INTO campus_fdw.persons (first_name, last_name, birth_date, contact_info, face_signature)
    VALUES ('Raul', 'Jimenez', '1979-08-20', 'raul820@yahoo.com', 'RJI002');

INSERT INTO campus_fdw.staff (hiring_date, title)
    VALUES ('08/10/2012', 'Social Studies Teacher');

UPDATE campus_fdw.staff
    SET (staff_person) = (
        SELECT gaia_id FROM campus_fdw.persons WHERE face_signature = 'RJI002')
    WHERE staff_person is null;

-- Staff - Science Teacher.
INSERT INTO campus_fdw.persons (first_name, last_name, birth_date, contact_info, face_signature)
    VALUES ('Helen', 'McDonald', '1970-08-20', 'helen@campus.com', 'HMD003');

INSERT INTO campus_fdw.staff (hiring_date, title)
    VALUES ('09/01/1999', 'Science Teacher');

UPDATE campus_fdw.staff
    SET (staff_person) = (
        SELECT gaia_id FROM campus_fdw.persons WHERE face_signature = 'HMD003')
    WHERE staff_person is null;

-- Staff - Adams Family - guardians (Susan & Henry) and students (Rachel and Michael).
INSERT INTO campus_fdw.persons (first_name, last_name, birth_date, contact_info, face_signature)
    VALUES ('Susan', 'Adams', '', 'susan_benson@gmail.com', 'SAD010');

INSERT INTO campus_fdw.guardians (relationship)
    VALUES ('mother');

UPDATE campus_fdw.guardians
    SET (guardian_person) = (
        SELECT gaia_id FROM campus_fdw.persons WHERE face_signature = 'SAD010')
    WHERE guardian_person is null;

INSERT INTO campus_fdw.persons (first_name, last_name, birth_date, contact_info, face_signature)
    VALUES ('Henry', 'Adams', '', 'henry.adams@gmail.com', 'HAD011');

INSERT INTO campus_fdw.guardians (relationship)
    VALUES ('father');

UPDATE campus_fdw.guardians
    SET (guardian_person) = (
        SELECT gaia_id FROM campus_fdw.persons WHERE face_signature = 'HAD011')
    WHERE guardian_person is null;

INSERT INTO campus_fdw.persons (first_name, last_name, birth_date, contact_info, face_signature)
    VALUES ('Rachel', 'Adams', '2006-12-14', 'rachel.a6@gmail.com', 'RAD012');

INSERT INTO campus_fdw.students (student_person, primary_guardian, secondary_guardian)
    SELECT p.gaia_id, g1.gaia_id, g2.gaia_id
    FROM campus_fdw.persons p, campus_fdw.persons p1, campus_fdw.persons p2, campus_fdw.guardians g1, campus_fdw.guardians g2
    WHERE p.face_signature = 'RAD012'
        AND p1.face_signature = 'SAD010' AND g1.guardian_person = p1.gaia_id
        AND p2.face_signature = 'HAD011' AND g2.guardian_person = p2.gaia_id;

INSERT INTO campus_fdw.persons (first_name, last_name, birth_date, contact_info, face_signature)
    VALUES ('Michael', 'Adams', '2010-07-24', 'mike456@gmail.com', 'MAD013');

INSERT INTO campus_fdw.students (student_person, primary_guardian, secondary_guardian)
    SELECT p.gaia_id, g1.gaia_id, g2.gaia_id
    FROM campus_fdw.persons p, campus_fdw.persons p1, campus_fdw.persons p2, campus_fdw.guardians g1, campus_fdw.guardians g2
    WHERE p.face_signature = 'MAD013'
        AND p1.face_signature = 'SAD010' AND g1.guardian_person = p1.gaia_id
        AND p2.face_signature = 'HAD011' AND g2.guardian_person = p2.gaia_id;

-- Staff - Subramanian Family - guardians (Saira & Balaji) and students (Aditi and Rajesh).
INSERT INTO campus_fdw.persons (first_name, last_name, birth_date, contact_info, face_signature)
    VALUES ('Saira', 'Subramanian', '1980-12-04', 'saira80@gmail.com', 'SSU020');

INSERT INTO campus_fdw.guardians (relationship)
    VALUES ('mother');

UPDATE campus_fdw.guardians
    SET (guardian_person) = (
        SELECT gaia_id FROM campus_fdw.persons WHERE face_signature = 'SSU020')
    WHERE guardian_person is null;

INSERT INTO campus_fdw.persons (first_name, last_name, birth_date, contact_info, face_signature)
    VALUES ('Balaji', 'Subramanian', '1977-02-20', 'balaji77@gmail.com', 'BSU021');

INSERT INTO campus_fdw.guardians (relationship)
    VALUES ('father');

UPDATE campus_fdw.guardians
    SET (guardian_person) = (
        SELECT gaia_id FROM campus_fdw.persons WHERE face_signature = 'BSU021')
    WHERE guardian_person is null;

INSERT INTO campus_fdw.persons (first_name, last_name, birth_date, contact_info, face_signature)
    VALUES ('Aditi', 'Subramanian', '2007-01-21', 'aditi124@outlook.com', 'ASU022');

INSERT INTO campus_fdw.students (student_person, primary_guardian, secondary_guardian)
    SELECT p.gaia_id, g1.gaia_id, g2.gaia_id
    FROM campus_fdw.persons p, campus_fdw.persons p1, campus_fdw.persons p2, campus_fdw.guardians g1, campus_fdw.guardians g2
    WHERE p.face_signature = 'ASU022'
        AND p1.face_signature = 'SSU020' AND g1.guardian_person = p1.gaia_id
        AND p2.face_signature = 'BSU021' AND g2.guardian_person = p2.gaia_id;

INSERT INTO campus_fdw.persons (first_name, last_name, birth_date, contact_info, face_signature)
    VALUES ('Rajesh', 'Subramanian', '2010-05-12', 'rajesh220@outlook.com', 'RSU023');

INSERT INTO campus_fdw.students (student_person, primary_guardian, secondary_guardian)
    SELECT p.gaia_id, g1.gaia_id, g2.gaia_id
    FROM campus_fdw.persons p, campus_fdw.persons p1, campus_fdw.persons p2, campus_fdw.guardians g1, campus_fdw.guardians g2
    WHERE p.face_signature = 'RSU023'
        AND p1.face_signature = 'BSU021' AND g1.guardian_person = p1.gaia_id
        AND p2.face_signature = 'SSU020' AND g2.guardian_person = p2.gaia_id;

-- Insert buildings and rooms.

-- Administration.
INSERT INTO campus_fdw.buildings (name, code)
    VALUES ('Administration Building', 'A1');

INSERT INTO campus_fdw.rooms (name, code, purpose, floor, capacity)
    VALUES ('Conference Room', 'A1CR2/1', 'meetings', 2, 50);

UPDATE campus_fdw.rooms
    SET (room_building) = (
        SELECT gaia_id FROM campus_fdw.buildings WHERE code = 'A1')
    WHERE room_building is null;

-- Library.
INSERT INTO campus_fdw.buildings (name, code)
    VALUES ('Mark Twain Library', 'L1');

INSERT INTO campus_fdw.rooms (name, code, purpose, floor, capacity)
    VALUES ('Conference Room', 'L1CR1/1', 'meetings', 1, 50);

UPDATE campus_fdw.rooms
    SET (room_building) = (
        SELECT gaia_id FROM campus_fdw.buildings WHERE code = 'L1')
    WHERE room_building is null;

-- Main Coursework Building.
INSERT INTO campus_fdw.buildings (name, code)
    VALUES ('Richard Bronson Hall', 'C1');

INSERT INTO campus_fdw.rooms (name, code, purpose, floor, capacity)
    VALUES ('Class Room 1/1', 'C1CR1/1', 'classes', 1, 40);

UPDATE campus_fdw.rooms
    SET (room_building) = (
        SELECT gaia_id FROM campus_fdw.buildings WHERE code = 'C1')
    WHERE room_building is null;

INSERT INTO campus_fdw.rooms (name, code, purpose, floor, capacity)
    VALUES ('Class Room 1/2', 'C1CR1/2', 'classes', 1, 40);

UPDATE campus_fdw.rooms
    SET (room_building) = (
        SELECT gaia_id FROM campus_fdw.buildings WHERE code = 'C1')
    WHERE room_building is null;

INSERT INTO campus_fdw.rooms (name, code, purpose, floor, capacity)
    VALUES ('Chemistry Laboratory', 'C1LR1/1', 'laboratory', 1, 40);

UPDATE campus_fdw.rooms
    SET (room_building) = (
        SELECT gaia_id FROM campus_fdw.buildings WHERE code = 'C1')
    WHERE room_building is null;

INSERT INTO campus_fdw.rooms (name, code, purpose, floor, capacity)
    VALUES ('Class Room 2/1', 'C1CR2/1', 'classes', 1, 60);

UPDATE campus_fdw.rooms
    SET (room_building) = (
        SELECT gaia_id FROM campus_fdw.buildings WHERE code = 'C1')
    WHERE room_building is null;

INSERT INTO campus_fdw.rooms (name, code, purpose, floor, capacity)
    VALUES ('Physics Laboratory', 'C1LR2/1', 'laboratory', 2, 40);

UPDATE campus_fdw.rooms
    SET (room_building) = (
        SELECT gaia_id FROM campus_fdw.buildings WHERE code = 'C1')
    WHERE room_building is null;

-- Sports Arena.
INSERT INTO campus_fdw.buildings (name, code)
    VALUES ('Arnold Schwarzenegger Arena', 'S1');

INSERT INTO campus_fdw.rooms (name, code, purpose, floor, capacity)
    VALUES ('Arena', 'S1M1', 'multipurpose', 1, 300);

UPDATE campus_fdw.rooms
    SET (room_building) = (
        SELECT gaia_id FROM campus_fdw.buildings WHERE code = 'S1')
    WHERE room_building is null;

-- Insert a few events with coordinators and attendees.

INSERT INTO campus_fdw.events (name, topic, agenda, date, begin_time, end_time)
    VALUES ('7th Grade Parents-Teacher Conference', 'status', 'Discuss students progress.', '2021-01-18', 1900, 2000);

UPDATE campus_fdw.events
    SET (event_room) = (
        SELECT gaia_id FROM campus_fdw.rooms WHERE code = 'C1CR1/1')
    WHERE event_room is null;

INSERT INTO campus_fdw.event_coordination (coordinator, coordinated_event)
    SELECT p.gaia_id, e.gaia_id
    FROM campus_fdw.persons p, campus_fdw.events e
    WHERE p.face_signature = 'HMD003' AND e.name = '7th Grade Parents-Teacher Conference';

INSERT INTO campus_fdw.event_attendance (attendee, attended_event)
    SELECT p.gaia_id, e.gaia_id
    FROM campus_fdw.persons p, campus_fdw.events e
    WHERE p.face_signature = 'SAD010' AND e.name = '7th Grade Parents-Teacher Conference';

INSERT INTO campus_fdw.event_attendance (attendee, attended_event)
    SELECT p.gaia_id, e.gaia_id
    FROM campus_fdw.persons p, campus_fdw.events e
    WHERE p.face_signature = 'SSU020' AND e.name = '7th Grade Parents-Teacher Conference';

INSERT INTO campus_fdw.events (name, topic, agenda, date, begin_time, end_time)
    VALUES ('4th Grade Parents-Teacher Conference', 'status', 'Discuss students progress.', '2021-01-19', 1900, 2000);

UPDATE campus_fdw.events
    SET (event_room) = (
        SELECT gaia_id FROM campus_fdw.rooms WHERE code = 'C1CR1/1')
    WHERE event_room is null;

INSERT INTO campus_fdw.event_coordination (coordinator, coordinated_event)
    SELECT p.gaia_id, e.gaia_id
    FROM campus_fdw.persons p, campus_fdw.events e
    WHERE p.face_signature = 'RJI002' AND e.name = '4th Grade Parents-Teacher Conference';

INSERT INTO campus_fdw.event_attendance (attendee, attended_event)
    SELECT p.gaia_id, e.gaia_id
    FROM campus_fdw.persons p, campus_fdw.events e
    WHERE p.face_signature = 'HAD011' AND e.name = '4th Grade Parents-Teacher Conference';

INSERT INTO campus_fdw.event_attendance (attendee, attended_event)
    SELECT p.gaia_id, e.gaia_id
    FROM campus_fdw.persons p, campus_fdw.events e
    WHERE p.face_signature = 'BSU021' AND e.name = '4th Grade Parents-Teacher Conference';

INSERT INTO campus_fdw.events (name, topic, agenda, date, begin_time, end_time)
    VALUES ('Student Winter Show', 'show', 'Theater play.', '2020-12-21', 1900, 2100);

UPDATE campus_fdw.events
    SET (event_room) = (
        SELECT gaia_id FROM campus_fdw.rooms WHERE code = 'S1M1')
    WHERE event_room is null;

INSERT INTO campus_fdw.event_coordination (coordinator, coordinated_event)
    SELECT p.gaia_id, e.gaia_id
    FROM campus_fdw.persons p, campus_fdw.events e
    WHERE p.face_signature = 'GPH000' AND e.name = 'Student Winter Show';

INSERT INTO campus_fdw.event_coordination (coordinator, coordinated_event)
    SELECT p.gaia_id, e.gaia_id
    FROM campus_fdw.persons p, campus_fdw.events e
    WHERE p.face_signature = 'JVA001' AND e.name = 'Student Winter Show';

INSERT INTO campus_fdw.event_attendance (attendee, attended_event)
    SELECT p.gaia_id, e.gaia_id
    FROM campus_fdw.persons p, campus_fdw.events e
    WHERE p.face_signature = 'RJI002' AND e.name = 'Student Winter Show';

INSERT INTO campus_fdw.event_attendance (attendee, attended_event)
    SELECT p.gaia_id, e.gaia_id
    FROM campus_fdw.persons p, campus_fdw.events e
    WHERE p.face_signature = 'HMD003' AND e.name = 'Student Winter Show';

INSERT INTO campus_fdw.event_attendance (attendee, attended_event)
    SELECT p.gaia_id, e.gaia_id
    FROM campus_fdw.persons p, campus_fdw.events e
    WHERE p.face_signature = 'SAD010' AND e.name = 'Student Winter Show';

INSERT INTO campus_fdw.event_attendance (attendee, attended_event)
    SELECT p.gaia_id, e.gaia_id
    FROM campus_fdw.persons p, campus_fdw.events e
    WHERE p.face_signature = 'RAD012' AND e.name = 'Student Winter Show';

INSERT INTO campus_fdw.event_attendance (attendee, attended_event)
    SELECT p.gaia_id, e.gaia_id
    FROM campus_fdw.persons p, campus_fdw.events e
    WHERE p.face_signature = 'MAD013' AND e.name = 'Student Winter Show';

INSERT INTO campus_fdw.event_attendance (attendee, attended_event)
    SELECT p.gaia_id, e.gaia_id
    FROM campus_fdw.persons p, campus_fdw.events e
    WHERE p.face_signature = 'SSU020' AND e.name = 'Student Winter Show';

INSERT INTO campus_fdw.event_attendance (attendee, attended_event)
    SELECT p.gaia_id, e.gaia_id
    FROM campus_fdw.persons p, campus_fdw.events e
    WHERE p.face_signature = 'ASU022' AND e.name = 'Student Winter Show';

INSERT INTO campus_fdw.event_attendance (attendee, attended_event)
    SELECT p.gaia_id, e.gaia_id
    FROM campus_fdw.persons p, campus_fdw.events e
    WHERE p.face_signature = 'RSU023' AND e.name = 'Student Winter Show';
