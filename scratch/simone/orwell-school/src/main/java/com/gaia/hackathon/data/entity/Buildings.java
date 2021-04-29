package com.gaia.hackathon.data.entity;

import lombok.Data;
import lombok.EqualsAndHashCode;

import javax.persistence.Column;
import javax.persistence.Entity;
import javax.persistence.Table;
import java.io.Serializable;

@Entity
@Data
@EqualsAndHashCode(callSuper = true)
@Table(name = "buildings", schema = "school_fdw")
public class Buildings extends com.gaia.hackathon.data.AbstractEntity implements Serializable {

    private static final long serialVersionUID = 1L;


    @Column(name = "building_name")
    private String buildingName;

    @Column(name = "camera_id")
    private Long cameraId;

    @Column(name = "door_closed")
    private String doorClosed;

}
