package com.gaia.hackathon.data.entity;

import lombok.Data;
import lombok.EqualsAndHashCode;

import javax.persistence.Column;
import javax.persistence.Entity;
import javax.persistence.Table;
import java.io.Serializable;

@Table(name = "persons", schema = "school_fdw")
@Entity
@Data
@EqualsAndHashCode(callSuper = true)
public class Persons extends com.gaia.hackathon.data.AbstractEntity implements Serializable {

    private static final long serialVersionUID = 1L;

    @Column(name = "first_name")
    private String firstName;

    @Column(name = "last_name")
    private String lastName;

    @Column(name = "birth_date")
    private Long birthDate;

    @Column(name = "face_signature")
    private Long faceSignature;

    @Column(name = "person_type")
    private Integer personType;

}
