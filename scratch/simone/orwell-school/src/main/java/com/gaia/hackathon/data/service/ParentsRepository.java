package com.gaia.hackathon.data.service;

import com.gaia.hackathon.data.entity.Parents;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.JpaSpecificationExecutor;

public interface ParentsRepository extends JpaRepository<Parents, Void>, JpaSpecificationExecutor<Parents> {

}