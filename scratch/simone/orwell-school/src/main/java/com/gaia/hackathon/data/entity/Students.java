package com.gaia.hackathon.data.entity;

import lombok.Data;
import lombok.EqualsAndHashCode;

import javax.persistence.Column;
import javax.persistence.Entity;
import javax.persistence.Table;
import java.io.Serializable;

@Entity
@Table(name = "students")
@Data
@EqualsAndHashCode(callSuper = true)
public class Students extends com.gaia.hackathon.data.AbstractEntity implements Serializable {

    private static final long serialVersionUID = 1L;


    @Column(name = "telephone")
    private String telephone;

    @Column(name = "student_person")
    private Long studentPerson;

}
