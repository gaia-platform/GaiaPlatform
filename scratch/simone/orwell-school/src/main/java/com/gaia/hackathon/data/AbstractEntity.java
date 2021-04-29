package com.gaia.hackathon.data;

import lombok.Getter;
import lombok.Setter;

import javax.persistence.Column;
import javax.persistence.GeneratedValue;
import javax.persistence.Id;
import javax.persistence.MappedSuperclass;

@MappedSuperclass
public abstract class AbstractEntity {

    @Id
    @Getter
    @Setter
    @Column(name = "gaia_id")
    private Integer gaiaId;

    @Override
    public int hashCode() {
        if (gaiaId != null) {
            return gaiaId.hashCode();
        }
        return super.hashCode();
    }

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof AbstractEntity)) {
            return false; // null or other class
        }
        AbstractEntity other = (AbstractEntity) obj;

        if (gaiaId != null) {
            return gaiaId.equals(other.gaiaId);
        }
        return super.equals(other);
    }
}