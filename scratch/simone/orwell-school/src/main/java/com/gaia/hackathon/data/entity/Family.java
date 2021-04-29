package com.gaia.hackathon.data.entity;

import lombok.Data;
import lombok.EqualsAndHashCode;

import javax.persistence.Column;
import javax.persistence.Entity;
import javax.persistence.Table;
import java.io.Serializable;

@Entity
@Table(name = "family")
@Data
@EqualsAndHashCode(callSuper = true)
public class Family extends com.gaia.hackathon.data.AbstractEntity implements Serializable {

    private static final long serialVersionUID = 1L;


    @Column(name = "mother")
    private Long mother;

    @Column(name = "father")
    private Long father;

    @Column(name = "student")
    private Long student;

}
