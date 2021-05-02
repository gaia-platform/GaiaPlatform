package com.gaia.hackathon.data.entity;

import lombok.Data;
import lombok.EqualsAndHashCode;

import javax.persistence.Column;
import javax.persistence.Entity;
import javax.persistence.Table;
import java.io.Serializable;

@Entity
@Table(name = "rooms", schema = "school_fdw")
@Data
@EqualsAndHashCode(callSuper = true)
public class Rooms extends com.gaia.hackathon.data.AbstractEntity implements Serializable {

    private static final long serialVersionUID = 1L;


    @Column(name = "room_number")
    private Long roomNumber;

    @Column(name = "room_name")
    private String roomName;

    @Column(name = "floor_number")
    private Long floorNumber;

    @Column(name = "capacity")
    private Long capacity;

    @Column(name = "building")
    private Long building;

}
