package com.gaia.hackathon.data.entity;

import lombok.Data;
import lombok.EqualsAndHashCode;

import javax.persistence.Column;
import javax.persistence.Entity;
import javax.persistence.Table;
import java.io.Serializable;

@Entity
@Table(name = "events", schema = "school_fdw")
@Data
@EqualsAndHashCode(callSuper = true)
public class Events extends com.gaia.hackathon.data.AbstractEntity implements Serializable {

    private static final long serialVersionUID = 1L;

    @Column(name = "name")
    private String name;

    @Column(name = "start_date")
    private Long startTime;

    @Column(name = "end_time")
    private Long endTime;

    @Column(name = "event_actually_enrolled")
    private String eventActuallyEnrolled;

    @Column(name = "enrollment")
    private Long enrollment;

    @Column(name = "room")
    private Long room;

    @Column(name = "teacher")
    private Long teacher;

}
