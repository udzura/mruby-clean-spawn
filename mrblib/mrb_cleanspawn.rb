module CleanSpawn
  def self.cgroup_root_path
    @cgroup_root_path || nil
  end

  def self.cgroup_root_path=(newpath)
    @cgroup_root_path = newpath
  end
end
