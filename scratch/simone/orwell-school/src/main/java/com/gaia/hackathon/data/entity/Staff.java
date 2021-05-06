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
@Table(name = "staff", schema = "school_fdw")
public class Staff extends com.gaia.hackathon.data.AbstractEntity implements Serializable {

    private static final long serialVersionUID = 1L;


    @Column(name = "hired_date")
    private Long hiredDate;

    @Column(name = "person")
    private Long person;

}
