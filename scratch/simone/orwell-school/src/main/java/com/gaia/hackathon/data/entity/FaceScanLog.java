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
@Table(name = "face_scan_log")
public class FaceScanLog extends com.gaia.hackathon.data.AbstractEntity implements Serializable {

    private static final long serialVersionUID = 1L;


    @Column(name = "scan_signature")
    private Long scanSignature;

    @Column(name = "scan_date")
    private Long scanDate;

    @Column(name = "person_type")
    private Integer personType;

    @Column(name = "building_log")
    private Long buildingLog;

    @Column(name = "singature_id")
    private Long singatureId;

}
