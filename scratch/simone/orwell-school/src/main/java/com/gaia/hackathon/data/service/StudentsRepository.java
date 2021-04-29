package com.gaia.hackathon.data.service;

import com.gaia.hackathon.data.entity.Students;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.JpaSpecificationExecutor;

public interface StudentsRepository extends JpaRepository<Students, Void>, JpaSpecificationExecutor<Students> {

}