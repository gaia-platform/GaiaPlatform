package com.gaia.hackathon.data.entity;

import lombok.Data;
import lombok.EqualsAndHashCode;

import javax.persistence.Column;
import javax.persistence.Entity;
import javax.persistence.Table;
import java.io.Serializable;

@Entity
@Table(name = "parents")
@Data
@EqualsAndHashCode(callSuper = true)
public class Parents extends com.gaia.hackathon.data.AbstractEntity implements Serializable {

    private static final long serialVersionUID = 1L;

    @Column(name = "parent_type")
    private String parentType;

    @Column(name = "parent_person")
    private Long parentPerson;

}
