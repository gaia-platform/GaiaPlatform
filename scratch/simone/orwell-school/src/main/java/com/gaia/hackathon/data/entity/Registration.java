package com.gaia.hackathon.data.entity;

import lombok.Data;
import lombok.EqualsAndHashCode;

import javax.persistence.Column;
import javax.persistence.Entity;
import javax.persistence.Table;
import java.io.Serializable;

@Entity
@Table(name = "registration")
@Data
@EqualsAndHashCode(callSuper = true)
public class Registration extends com.gaia.hackathon.data.AbstractEntity implements Serializable {

    private static final long serialVersionUID = 1L;


    @Column(name = "registration_date")
    private Long registrationDate;

    @Column(name = "event")
    private Long event;

    @Column(name = "reg_student")
    private Long regStudent;

}
